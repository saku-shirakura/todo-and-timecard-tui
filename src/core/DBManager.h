// MIT License
//
// Copyright (c) saku shirakura <saku@sakushira.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/**
 * @file DBManager.h
 * @date 25/05/02
 * @brief ファイルの説明
 * @details ファイルの詳細
 * @author saku shirakura (saku@sakushira.com)
 */


#ifndef DBMANAGER_H
#define DBMANAGER_H
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <sqlite3.h>
#include <vector>

namespace core::db {
    enum class ColType {
        T_REAL,
        T_INTEGER,
        T_TEXT,
        T_NULL
    };

    using ColValue = std::pair<ColType, std::variant<double, long long, std::string, std::nullptr_t>>;

    using RowHash = std::unordered_map<std::string, ColValue>;

    using Table = std::vector<RowHash>;
    /**
     * @brief データベース接続を管理し、SQL文を実行します。
     */
    class DBManager final {
    public:
        DBManager() = default;

        ~DBManager();

        /**
         * @brief デフォルトのデータベースファイルのパスを変更します。
         * @param file_path_ 接続対象データベースのパス
         */
        static void setDBFile(std::string file_path_);

        /**
         * @brief DBManager全体でのデータベース接続を確立します。
         * @details _db_file_pathは絶対パスでファイルを指してください。
         * @returns 正常に終了した場合は、0を返します。エラーコードは以下を参照してください。
         * @returns -1: 絶対パスではありません。
         * @returns -2: パスがファイルを指していません。
         * @returns -3: パスは通常のファイル以外を指しています。
         * @returns その他のエラーは、_getPrefixedErrorCode()によるエラーを返します。
         */
        static int openDB();


        /**
         * @brief sql文を実行します。
         * @param sql_ 実行するsql文
         * @param callback_ コールバック関数
         * @param callback_arg コールバック関数の第一引数
         * @returns 戻り値については、openDB()を参照してください。
         */
        static int execute(const std::string& sql_, int (*callback_)(void*, int, char**, char**), void* callback_arg);

        /**
         * @brief 単一のsql文を実行します。その際、binder_コールバックによりプレースホルダを利用できます。
         * @param sql_ 単一のsql文
         * @param result_table_ 値を格納する対象のテーブル。emplace_backにより要素が追加されます。
         * @param binder_ placeholderをバインドするためのコールバック (prepareの実行直後に呼び出されます。)
         * @param binder_arg_
         * @return 戻り値については、openDB()を参照してください。
         */
        static int usePlaceholderUniSql(const std::string& sql_,
                                        Table& result_table_,
                                        int (*binder_)(void*, sqlite3_stmt*), void* binder_arg_);

        /**
         * @details INVALID_PREFIX プリフィックスは付与されていないか無効です。
         * @details OPEN_DB_ERROR データベース接続時のエラー
         * @details INITIALIZE_DB_ERROR データベース初期化時のエラー
         * @details EXECUTE_ERROR sqlite3_exec実行時のエラー
         * @details PREPARE_SQL_ERROR sqlite3_prepare_v2のエラー
         * @details STEP_ERROR sqlite3_stepのエラー
         * @details FINALIZE_STMT_ERROR sqlite3_finalizeのエラー
         */
        enum class ErrorPrefix {
            INVALID_PREFIX = 0,
            OPEN_DB_ERROR = 1,
            INITIALIZE_DB_ERROR = 2,
            EXECUTE_ERROR = 3,
            PREPARE_SQL_ERROR = 4,
            STEP_ERROR = 5,
            FINALIZE_STMT_ERROR = 6,
            BIND_ERROR = 7
        };


        /**
         * @brief 実際のエラーコードを取得します。
         * @param error_code_ 発生個所を含んだエラーコード
         * @return SQLITE等から発せられたエラーコード
         */
        static int getErrorCode(int error_code_);

        /**
         * @brief エラーコードに付与されたエラーの発生個所を取得します。
         * @param error_code_ 発生個所を含んだエラーコード
         * @return 発生個所
         */
        static ErrorPrefix getErrorPos(int error_code_);

        /**
         * @brief エラーの発生個所をエラーコードに付与します。
         * @details エラー情報は、getErrorCode(), getErrorPos()により取り出せます。
         * @details 通常は、SQLITEの定数`SQLITE_XXX`いずれのエラーコードです。
         * @param error_code_ SQLITEのエラーコード
         * @param prefix_ エラー発生個所を表すプリフィックス。
         * @return error_code_にprefix_付与した値。
         */
        static int getPrefixedErrorCode(int error_code_, ErrorPrefix prefix_);

    private:
        /**
         * @brief データベース`db_file_`を開きます。
         * @param db_file_ 接続したいデータベースファイル
         * @return _getPrefixedErrorCode()に従ったエラーを返します。
         */
        int _openDB(const std::string& db_file_);

        /**
         * @brief DBを初期化します。
         * @details 初期化時に実行するsql文は、`resource/initialize_db.sql`にあります。
         * @details [file_bundler](https://github.com/ln3-net/file_bundler)によりバンドルされました。
         * @details コマンド(プロジェクトルートにて): `file_bundler -i ./resource -o ./src -y`
         * @return _getPrefixedErrorCode()に従ったエラーを返します。
         */
        [[nodiscard]] int _initializeDB() const;

        sqlite3* _db{nullptr};
        static std::unique_ptr<DBManager> _manager;
        static std::filesystem::path _db_file_path;
    };

    class DatabaseTable {
    public:
        DatabaseTable() = default;
        ~DatabaseTable() = default;

        DatabaseTable(std::vector<std::string> column_names_,
                      std::string table_name_);

        int selectRecords();

        /**
         *
         * @param where_condition_
         * @param placeholder_value_
         * @return
         */
        int selectRecords(const std::string& where_condition_,
                          std::vector<ColValue> placeholder_value_);

        /**
         *
         * @return
         */
        const Table& getTable();

    protected:

        /**
         *
         * @param bind_arg_
         * @param stmt
         * @return
         */
        static int _binder(void* bind_arg_, sqlite3_stmt* stmt);

        std::vector<std::string> _column_names;

        std::string _table_name;

        Table _data;
    };

    class Tables {
    public:
        static DatabaseTable Status;
        static DatabaseTable Task;
        static DatabaseTable Worktime;
        static DatabaseTable Schedule;
    };
} // core

#endif //DBMANAGER_H
