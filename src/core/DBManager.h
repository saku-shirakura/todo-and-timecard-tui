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
#include <format>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <sqlite3.h>
#include <vector>
#include <mutex>
#include <chrono>

/**
 * @note getDouble(), getLongLong(), getString()等の仕様によりNULLは、0, 空文字などとして扱われます。
 */
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

    namespace sqliteDeleter {
        struct DatabaseCloser {
            void operator()(sqlite3* db_) const;
        };

        struct StatementFinalizer {
            void operator()(sqlite3_stmt* stmt_) const;
        };

        struct SqliteStringDeleter {
            void operator()(char* string_) const;
        };
    }

    /**
     * @brief ColValueから適切な形式で値を取得します。
     * @param value_ 値が格納されているColValue
     * @param default_value_ 保有している値が関数の引数と異なる場合に使用されます。
     * @return 取得した値又はdefault_value_
     */
    double getDouble(const ColValue& value_, double default_value_ = 0.0);

    /**
     * @brief getDouble()を参照してください。
     * @param value_
     * @param default_value_
     * @return
     */
    long long getLongLong(const ColValue& value_, long long default_value_ = 0);

    /**
     * @brief getDouble()を参照してください。
     * @param value_
     * @param default_value_
     * @return
     */
    std::string getString(const ColValue& value_, const std::string& default_value_ = "");

    /**
     * @brief データベース接続を管理し、SQL文を実行します。
     */
    class DBManager final {
    public:
        DBManager() = default;

        ~DBManager() = default;

        /**
         * @brief デフォルトのデータベースファイルのパスを変更します。この際、接続は閉じられます。
         * @param file_path_ 接続対象データベースのパス
         */
        static bool setDBFile(const std::string& file_path_);

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
         * @brief 複数のsql文を実行します。データは取得できません。データを取得する用途では`usePlaceholderUniSql()`を使用してください。
         * @param sql_ 実行するsql文
         * @returns 戻り値については、openDB()を参照してください。
         */
        static int execute(const std::string& sql_);

        /**
         * @brief 先頭のsql文を実行します。その際、binder_コールバックによりプレースホルダを利用できます。
         * @details この関数に無効なsql文が渡され、sqlite3_prepare_v2()による、準備済みステートメントがnullptrである場合はEND_OF_STATEMENTを返します。
         * @param sql_ sql文
         * @param result_table_ 値を格納する対象のテーブル。emplace_backにより要素が追加されます。
         * @param binder_ placeholderをバインドするためのコールバック (prepareの実行直後に呼び出されます。)
         * @param binder_arg_ binderの第一引数
         * @param sql_remaining_ sql_のうち実行されなかった部分の文字列
         * @return 戻り値については、openDB()を参照してください。
         */
        static int usePlaceholderUniSql(const std::string& sql_,
                                        Table& result_table_,
                                        int (*binder_)(void*, sqlite3_stmt*), void* binder_arg_,
                                        std::string& sql_remaining_);

        /**
         * @details FIRST_ENUM 先頭の値 - 1
         * @details INVALID_PREFIX プリフィックスは付与されていないか無効です。
         * @details OPEN_DB_ERROR データベース接続時のエラー
         * @details INITIALIZE_DB_ERROR データベース初期化時のエラー
         * @details EXECUTE_ERROR sqlite3_exec実行時のエラー
         * @details PREPARE_SQL_ERROR sqlite3_prepare_v2のエラー
         * @details STEP_ERROR sqlite3_stepのエラー
         * @details FINALIZE_STMT_ERROR sqlite3_finalizeのエラー
         * @details BIND_ERROR sqlite3_bind_XXXのエラー
         * @details MAPPER_ERROR マッピングに失敗
         * @details CLOSE_ERROR DBを閉じられませんでした。
         * @details DB_NOT_OPEN DBは開かれていません。
         * @details END_OF_STATEMENT 処理可能なsql文はありません。
         * @details LAST_ENUM ErrorPrefixの数を求めるためのenum
         */
        enum class ErrorPrefix {
            INVALID_PREFIX = 0,
            OPEN_DB_ERROR,
            INITIALIZE_DB_ERROR,
            EXECUTE_ERROR,
            PREPARE_SQL_ERROR,
            STEP_ERROR,
            STMT_ERROR,
            BIND_ERROR,
            MAPPER_ERROR,
            CLOSE_ERROR,
            DB_NOT_OPEN,
            END_OF_STATEMENT,
            LAST_ENUM
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

        /**
         * @brief データベースのデータをすべて破棄し、初期化します。
         * @return openDB()の戻り値、もしくは、_closeDBの戻り値、もしくは、std::filesystem::removeのエラーコード
         */
        static int ReinitializeDB();

    private:
        /**
         * @brief error_pref_がErrorPrefixの範囲内か確認します。
         * @param error_pref_ 判定対象
         * @return 判定結果
         */
        static bool _isValidErrorPos(int error_pref_);

        /**
         * @brief 複数のsql文を実行します。データは取得できません。データを取得する用途では`usePlaceholderUniSql()`を使用してください。
         * @param sql_ 実行するsql文
         * @returns 戻り値については、openDB()を参照してください。
         */
        [[nodiscard]] int _execute(const std::string& sql_);

        /**
         * @brief 先頭のsql文を実行します。その際、binder_コールバックによりプレースホルダを利用できます。
         * @details この関数に無効なsql文が渡され、sqlite3_prepare_v2()による、準備済みステートメントがnullptrである場合はEND_OF_STATEMENTを返します。
         * @param sql_ sql文
         * @param result_table_ 値を格納する対象のテーブル。emplace_backにより要素が追加されます。
         * @param binder_ placeholderをバインドするためのコールバック (prepareの実行直後に呼び出されます。)
         * @param binder_arg_ binderの第一引数
         * @param sql_remaining_ sql_のうち実行されなかった部分の文字列
         * @return 戻り値については、openDB()を参照してください。
         */
        int _usePlaceholderUniSql(const std::string& sql_,
                                  Table& result_table_,
                                  int (*binder_)(void*, sqlite3_stmt*), void* binder_arg_,
                                  std::string& sql_remaining_);

        /**
         * @brief この関数は、_usePlaceholderUniSql()のロジックです。_interface_mtxをロックしない場合には、呼び出さないでください。
         * @note 引数や戻り値の詳細は_usePlaceholderUniSql()を確認してください。
         */
        int _usePlaceholderUniSqlInternal(std::string sql_,
                                          Table& result_table_,
                                          int (*binder_)(void*, sqlite3_stmt*), void* binder_arg_,
                                          std::string& sql_remaining_);

        /**
         * @brief データベース`db_file_`を開きます。
         * @param db_file_ 接続したいデータベースファイル
         * @return _getPrefixedErrorCode()に従ったエラーを返します。
         */
        int _openDB(const std::string& db_file_);

        /**
         * @brief データベース接続を閉じます。
         * @return _getPrefixedErrorCode()に従ったエラーを返します。
         */
        void _closeDB();

        /**
         * @brief DBを初期化します。
         * @details 初期化時に実行するsql文は、`resource/initialize_db.sql`にあります。
         * @details [file_bundler](https://github.com/ln3-net/file_bundler)によりバンドルされました。
         * @details コマンド(プロジェクトルートにて): `file_bundler -i ./resource -o ./src -y`
         * @return _getPrefixedErrorCode()に従ったエラーを返します。
         */
        [[nodiscard]] int _initializeDB();

        /**
         *
         * @param start_query_at_ クエリの実行開始時間
         * @param sql_ 実行されたsql文
         * @param success_ クエリは成功した？
         * @param is_selected trueであれば、選択クエリ、falseなら更新クエリとして扱います。
         * @param rows_count_ is_selectedがtrueなら選択された行数。そうでないなら、変更された行数。
         */
        static void _queryLogger(std::chrono::time_point<std::chrono::high_resolution_clock> start_query_at_,
                                 const std::string& sql_, bool success_, bool is_selected, size_t rows_count_);

        static std::unique_ptr<char, sqliteDeleter::SqliteStringDeleter> sqlite3ExpandedSqlWrapper(sqlite3_stmt* stmt_);

        std::unique_ptr<sqlite3, sqliteDeleter::DatabaseCloser> _db{nullptr, sqliteDeleter::DatabaseCloser()};
        static std::unique_ptr<DBManager> _manager;
        static std::filesystem::path _db_file_path;
        std::mutex _internal_mtx;
        std::mutex _interface_mtx;
    };

    class DatabaseTable {
    public:
        DatabaseTable() = delete;
        virtual ~DatabaseTable() = default;

        /**
         * @brief 操作対象の列名とテーブル名を指定するコンストラクタ。(executeでは使用されません。)
         * @param column_names_ 操作対象列名のリスト
         * @param table_name_ 操作対象テーブルの名称
         */
        DatabaseTable(std::vector<std::string> column_names_,
                      std::string table_name_);

        /**
         * @brief 先頭のSQL文を実行します。
         * @param sql_ 実行したいsql文 (1文のみ)
         * @param placeholder_value_ プレースホルダの値
         * @param sql_remaining_ SQL文の実行されなかった部分
         * @return 正常終了時は0を返します。0以外を返す場合、`getPrefixedErrorCode(sqlite_error, error_pos)`によって求められたエラーコードです。
         */
        int usePlaceholderUniSql(const std::string& sql_, std::vector<ColValue> placeholder_value_,
                                 std::string& sql_remaining_);

        /**
         * @brief 単一のSQL文を実行します。usePlaceholderUniSql(sql_, {}, std::string())のエイリアスです。
         * @note 詳細はusePlaceholderUniSql(sql_, placeholder_value_, sql_remaining_)を参照してください。
         */
        int usePlaceholderUniSql(const std::string& sql_, std::vector<ColValue> placeholder_value_ = {});

        /**
         * @brief 全てのレコードを取得します。
         * @return 正常終了時は0を返します。0以外を返す場合、`getPrefixedErrorCode(sqlite_error, error_pos)`によって求められたエラーコードです。
         */
        int selectRecords();

        /**
         * @brief 1つのselect文を実行し、メンバ変数_dataを更新する(結果はgetTable()によって取得できる)。その際、placeholderを使用する。
         * @param where_clause_ WHERE句の後の条件文
         * @param placeholder_value_ 条件文のplaceholder(?)に埋め込む値
         * @param order_by_ ORDER BY句の後の条件文
         * @return 正常終了時は0を返します。0以外を返す場合、`getPrefixedErrorCode(sqlite_error, error_pos)`によって求められたエラーコードです。
         */
        int selectRecords(const std::string& where_clause_,
                          const std::vector<ColValue>& placeholder_value_, const std::string& order_by_ = "");

        /**
         * @brief 1つのselect文を実行し、メンバ変数_dataを更新する(結果はgetTable()によって取得できる)。その際、placeholderを使用する。
         * @param where_clause_ WHERE句の後の条件文
         * @param placeholder_value_ 条件文のplaceholder(?)に埋め込む値
         * @param order_by_ ORDER BY句の後の条件文
         * @param limit_ LIMIT句の数値
         * @param offset_ OFFSET句の数値
         * @return 正常終了時は0を返します。0以外を返す場合、`getPrefixedErrorCode(sqlite_error, error_pos)`によって求められたエラーコードです。
         */
        int selectRecords(const std::string& where_clause_, const std::vector<ColValue>& placeholder_value_,
                          const std::string& order_by_, int limit_, int offset_);

        /**
         * @brief 生のテーブルを取得します。
         * @return 直前に取得されたテーブル。
         */
        const Table& getRawTable();

        /**
         * @brief 列名を取得します。
         * @return 列名
         */
        const std::vector<std::string>& getColumnNames();

        /**
         * @brief テーブル名を取得します。
         * @return テーブル名
         */
        const std::string& getTableName();

    protected:
        /**
         * @brief sqlite3_stmtに値をbindする。userPlaceholderUniSql()関数のbinder_コールバックとして使用します。
         * @param bind_arg_ `std::vector<ColValue>*` placeholderと対応したbind対象の変数のリスト
         * @param stmt bind対象のsqlite3_stmt
         * @return 正常終了時は0を返します。0以外を返す場合、`getPrefixedErrorCode(sqlite_error, error_pos)`によって求められたエラーコードです。
         */
        static int _binder(void* bind_arg_, sqlite3_stmt* stmt);

        /**
         * @details 派生クラスでのメンバ変数へのマッピング用途に使用できます。
         * @details この関数は、selectRecordsの後に取得成功の有無にかかわらず呼び出されます。
         */
        virtual void _mapper();

        std::vector<std::string> _column_names;

        std::string _table_name;

        Table _data;
    };

    enum class Status {
        Progress = 1,
        Incomplete = 2,
        Complete = 3,
        Not_Planned = 4,
    };

    class Task {
    public:
        const long long id;
        const long long parent_id;
        const std::string name;
        const std::string detail;
        const long long status_id;
        const long long created_at;
        const long long updated_at;

        friend class TaskTable;

    private:
        Task();

        Task(long long id_,
             long long parent_id_,
             std::string name_,
             std::string detail_,
             long long status_id_,
             long long created_at_,
             long long updated_at_
        );
    };

    class TaskTable final : public DatabaseTable {
    public:
        TaskTable();

        const std::unordered_map<long long, Task>& getTable() const;
        const std::vector<long long>& getKeys() const;

        /**
         * @brief ページング子タスクを取得する。
         * @param parent_task_id_ 対象の親タスクID。0以下の値が指定された場合、NULLとして扱います。
         * @param status_filter_ 対象のステータスID。列挙型Statusと対応しています。Statusに存在しない値が指定された場合は絞り込まれません。
         * @param page_ 対象のページ。負の数を指定した場合には、ページングされません。
         * @param per_page_ 1ページ当たりのアイテム数。負の数を指定した場合には、ページングされません。
         * @return <成功ステータス, 親タスク名> 成功ステータスで0以外の値が返った場合、エラーが発生しています。
         */
        std::pair<int, std::string> fetchChildTasks(long long parent_task_id_, int status_filter_, int page_ = -1,
                                                    int per_page_ = -1);

        /**
         * @brief 親タスク・ステータスで絞り込んだ子タスクの数を取得する。
         * @param parent_task_id_ 対象の親タスクID。0以下の値が指定された場合、NULLとして扱います。
         * @param filter_status_ 対象のステータスID。列挙型Statusと対応しています。Statusに存在しない値が指定された場合は絞り込まれません。
         * @return <成功ステータス, タスクの数> 成功ステータスで0以外の値が返った場合、エラーが発生しています。
         */
        static std::pair<int, long long> countChildTasks(long long parent_task_id_, int filter_status_);

        /**
         * @brief ステータスフィルタ適用時にタスクIDが属するページ番号を取得します。
         * @param task_id_ タスクID
         * @param status_filter_ ステータスフィルタ
         * @param per_page_ 1ページ当たりのタスク数
         * @return <成功ステータス, <ページ番号, ページ内での位置>> 成功ステータスで0以外の値が返った場合、エラーが発生しています。
         */
        static std::pair<int, std::pair<long long, long long>> fetchPageNumAndFocusFromTask(
            long long task_id_, int status_filter_, int per_page_);

        /**
         * @brief タスクIDから単一のタスクを取得します。
         * @param task_id_ 対象となるタスクID
         * @return <成功ステータス, タスク> 成功ステータスで0以外の値が返った場合、エラーが発生しています。
         */
        static std::pair<int, Task> fetchTask(long long task_id_);

        /**
         * @brief 合計作業時間を取得します。
         * @param task_id_ 取得対象のタスクID
         * @return <成功ステータス, 作業時間(秒)> 成功ステータスで0以外の値が返った場合、エラーが発生しています。
         */
        static std::pair<int, std::chrono::seconds> computeTotalWorktime(long long task_id_);

        static std::pair<int, std::chrono::seconds> fetchWorktime(long long task_id_);

        static std::pair<int, Task> fetchLastTask(long long parent_id);

        static int newTask(long long parent_id);

        static int deleteTask(long long task_id);

        static bool computeIsSiblings(long long sibling_task_id, long long parent_id);

    private:
        void _mapper() override;

        std::unordered_map<long long, Task> _table;
        std::vector<long long> _keys;
    };

    class Worktime {
    public:
        const long long id;
        const long long task_id;
        const long long starting_time;
        const long long finishing_time;
        const long long created_at;
        const long long updated_at;

        friend class WorktimeTable;

    private:
        Worktime(
            long long id_,
            long long task_id_,
            long long starting_time_,
            long long finishing_time_,
            long long created_at_,
            long long updated_at_
        );
    };

    class WorktimeTable final : public DatabaseTable {
    public:
        WorktimeTable();

        const std::unordered_map<long long, Worktime>& getTable() const;

        const std::vector<long long>& getKeys() const;

        static int ensureOnlyOneActiveTask();

        static int deactivateAllTasks();

        int selectActiveTask();

        static int activateTask(long long task_id_);

        static int updateWorktime(long long id_, const std::string& memo_);

    private:
        void _mapper() override;

        std::unordered_map<long long, Worktime> _table;
        std::vector<long long> _keys;
    };

    class Schedule {
    public:
        const long long id;
        const long long task_id;
        const long long starting_time;
        const long long finishing_time;
        const long long created_at;
        const long long updated_at;

        friend class ScheduleTable;

    private:
        Schedule(
            long long id_,
            long long task_id_,
            long long starting_time_,
            long long finishing_time_,
            long long created_at_,
            long long updated_at_);
    };

    class ScheduleTable final : public DatabaseTable {
    public:
        ScheduleTable();

        const std::unordered_map<long long, Schedule>& getTable() const;

        const std::vector<long long>& getKeys() const;

    private:
        void _mapper() override;

        std::unordered_map<long long, Schedule> _table;
        std::vector<long long> _keys;
    };

    class Migrate {
    public:
        const long long id;
        const long long applied;
        const long long created_at;
        const long long updated_at;

        friend class MigrateTable;

    private:
        Migrate(
            long long id_,
            long long applied_,
            long long created_at_,
            long long updated_at_
        );
    };

    class MigrateTable final : public DatabaseTable {
    public:
        MigrateTable();

        const std::unordered_map<long long, Migrate>& getTable() const;

        const std::vector<long long>& getKeys() const;

    private:
        void _mapper() override;

        std::unordered_map<long long, Migrate> _table;
        std::vector<long long> _keys;
    };

    class Setting final {
    public:
        const long long id;
        const std::string setting_key;
        const std::string value;
        const long long created_at;
        const long long updated_at;

        friend class SettingTable;

    private:
        Setting(
            long long id_,
            const std::string& setting_key_,
            const std::string& value_,
            long long created_at_,
            long long updated_at_
        );
    };

    class SettingTable final : public DatabaseTable {
    public:
        SettingTable();

        const std::unordered_map<long long, Setting>& getTable() const;

        const std::vector<long long>& getKeys() const;

    private:
        void _mapper() override;

        std::unordered_map<long long, Setting> _table;
        std::vector<long long> _keys;
    };
} // core

#endif //DBMANAGER_H
