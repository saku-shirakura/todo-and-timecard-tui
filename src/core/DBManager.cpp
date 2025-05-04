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

#include "DBManager.h"

#include <complex.h>
#include <format>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <variant>
#include <vector>

#include "../resource.h"

#include "../utilities/Utilities.h"

namespace core::db {
    DBManager::~DBManager()
    {
        if (_db != nullptr)
            sqlite3_close(_db);
        _db = nullptr;
    }

    void DBManager::setDBFile(std::string file_path_) { _db_file_path = std::move(file_path_); }

    int DBManager::openDB()
    {
        // データベース接続が既に開かれているなら、実行しない。
        if (_manager == nullptr) { _manager = std::make_unique<DBManager>(); }
        else { return 0; }
        if (!_db_file_path.is_absolute()) { return -1; }
        // ディレクトリを指している場合は実行しない。
        if (!_db_file_path.has_filename()) { return -2; }
        const bool exec_init = !std::filesystem::exists(_db_file_path.generic_string());
        // ディレクトリとファイルを作成。
        if (exec_init) {
            create_directories(_db_file_path.parent_path());
            std::ofstream file(_db_file_path);
            file.close();
        }
        // 通常のファイルでない場合は実行しない。
        if (!is_regular_file(_db_file_path)) { return -3; }
        // データベース接続を確立
        // 接続エラーを処理
        if (const int opendb_err = _manager->_openDB(_db_file_path.generic_string()); opendb_err != 0) {
            _manager = nullptr;
            return opendb_err;
        }
        // データベースを初期化。
        if (exec_init) {
            if (const int initialize_err = _manager->_initializeDB(); initialize_err != 0) { return initialize_err; }
        }
        return 0;
    }

    int DBManager::execute(const std::string& sql_, int (*callback_)(void*, int, char**, char**), void* callback_arg)
    {
        if (const int open_db_err = openDB(); open_db_err != 0) { return open_db_err; }
        if (const int execute_err = sqlite3_exec(_manager->_db, sql_.c_str(), callback_, callback_arg, nullptr);
            execute_err != 0) { return getPrefixedErrorCode(execute_err, ErrorPrefix::EXECUTE_ERROR); }
        return 0;
    }

    int DBManager::usePlaceholderUniSql(const std::string& sql_, Table& result_table_,
                                        int (*binder_)(void*, sqlite3_stmt*), void* binder_arg_)
    {
        // db接続を開く(既に開かれている場合は何も実行されない)
        if (const int open_db_err = openDB(); open_db_err != 0) { return open_db_err; }
        sqlite3_stmt* stmt;
        // sqlを準備する。
        if (const int prepare_err = sqlite3_prepare_v2(_manager->_db, sql_.c_str(), sql_.size(), &stmt, nullptr);
            prepare_err != SQLITE_OK) { return getPrefixedErrorCode(prepare_err, ErrorPrefix::PREPARE_SQL_ERROR); }
        // placeholderと値を紐づける。
        if (const int binder_err = binder_(binder_arg_, stmt); binder_err != SQLITE_OK) { return binder_err; }
        // sqlを実行
        int step_status = sqlite3_step(stmt);
        // 取得した行をTableに格納する。
        while (step_status == SQLITE_ROW) {
            const int col_count = sqlite3_column_count(stmt);
            RowHash current_row;
            // それぞれの列を適切な型でRowHashに格納する。
            for (int col = 0; col < col_count; col++) {
                const int col_type = sqlite3_column_type(stmt, col);
                ColValue col_val;
                switch (col_type) {
                case 2:
                    col_val.first = ColType::T_REAL;
                    col_val.second = sqlite3_column_double(stmt, col);
                    break;
                case 1:
                    col_val.first = ColType::T_INTEGER;
                    col_val.second = sqlite3_column_int64(stmt, col);
                    break;
                case 3:
                    {
                        col_val.first = ColType::T_TEXT;
                        // 戻り値がconst unsigned char*なので、const char*に変換する。
                        const auto text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
                        const int text_size = sqlite3_column_bytes(stmt, col);
                        col_val.second = std::string(text, text_size);
                        break;
                    }
                case 5:
                default:
                    col_val.first = ColType::T_NULL;
                    col_val.second = nullptr;
                    break;
                }
                const std::string column_name = sqlite3_column_name(stmt, col);
                current_row.try_emplace(column_name, col_val);
            }
            // TableにRowHashを追加。
            result_table_.emplace_back(std::move(current_row));
            // sqlを実行
            step_status = sqlite3_step(stmt);
        }
        if (step_status != SQLITE_DONE) { return getPrefixedErrorCode(step_status, ErrorPrefix::STEP_ERROR); }
        if (const int finalize_err = sqlite3_finalize(stmt); finalize_err != SQLITE_OK) {
            return getPrefixedErrorCode(finalize_err, ErrorPrefix::FINALIZE_STMT_ERROR);
        }
        return SQLITE_OK;
    }

    int DBManager::getErrorCode(const int error_code_) { return error_code_ % 100000; }

    DBManager::ErrorPrefix DBManager::getErrorPos(const int error_code_)
    {
        int err_pref = std::abs(error_code_) / 100000;
        if (err_pref > 6 || err_pref < 0) err_pref = 0;
        return static_cast<ErrorPrefix>(err_pref);
    }

    int DBManager::getPrefixedErrorCode(const int error_code_, ErrorPrefix prefix_)
    {
        return error_code_ + static_cast<int>(prefix_) * 100000;
    }

    int DBManager::_openDB(const std::string& db_file_)
    {
        if (this->_db == nullptr) {
            if (const int open_db_err = sqlite3_open(db_file_.c_str(), &this->_db); open_db_err != SQLITE_OK) {
                sqlite3_close(this->_db);
                return getPrefixedErrorCode(open_db_err, ErrorPrefix::OPEN_DB_ERROR);
            }
            const int execute_err = sqlite3_exec(
                this->_db, std::string(F_OPEN_DB_PREPROC_SQL, SIZE_OPEN_DB_PREPROC_SQL).c_str(), nullptr,
                nullptr, nullptr);
            if (execute_err != SQLITE_OK) { return getPrefixedErrorCode(execute_err, ErrorPrefix::EXECUTE_ERROR); }
        }
        return SQLITE_OK;
    }

    int DBManager::_initializeDB() const
    {
        const int sqlite_err = sqlite3_exec(
            this->_db,
            std::string(F_INITIALIZE_DB_SQL, SIZE_INITIALIZE_DB_SQL).c_str(),
            nullptr,
            nullptr,
            nullptr
        );
        return sqlite_err;
    }

    DatabaseTable::DatabaseTable(std::vector<std::string> column_names_, std::string table_name_):
        _column_names(std::move(column_names_)),
        _table_name(std::move(table_name_))
    {
    }

    int DatabaseTable::selectRecords() { return selectRecords("", {}); }

    int DatabaseTable::selectRecords(const std::string& where_condition_, std::vector<ColValue> placeholder_value_)
    {
        std::string columns;
        for (const std::string& col : _column_names) {
            if (!columns.empty())
                columns += ", ";
            columns += col;
        }
        std::string sql;
        if (where_condition_.empty())
            sql = std::format("SELECT {} FROM {};", columns, _table_name);
        else sql = std::format("SELECT {} FROM {} WHERE {};", columns, _table_name, where_condition_);
        _data.clear();
        if (const int err = DBManager::usePlaceholderUniSql(sql, _data, _binder, &placeholder_value_); err !=
            SQLITE_OK) { return err; }
        return SQLITE_OK;
    }

    const Table& DatabaseTable::getTable() { return _data; }

    int DatabaseTable::_binder(void* bind_arg_, sqlite3_stmt* stmt)
    {
        const auto bind_values = static_cast<std::vector<ColValue>*>(bind_arg_);
        int binder_err = 0;
        for (int i = 0; i < bind_values->size(); i++) {
            switch (auto [column_type, value] = bind_values->at(i); column_type) {
            case ColType::T_REAL:
                binder_err = sqlite3_bind_double(stmt, i, std::get<double>(value));
                break;
            case ColType::T_INTEGER:
                binder_err = sqlite3_bind_int64(stmt, i, std::get<long long>(value));
                break;
            case ColType::T_TEXT:
                binder_err = sqlite3_bind_text(stmt, i, std::get<std::string>(value).c_str(), -1, SQLITE_STATIC);
                break;
            case ColType::T_NULL:
                binder_err = sqlite3_bind_null(stmt, i);
                break;
            }
            if (binder_err != SQLITE_OK) {
                return DBManager::getPrefixedErrorCode(binder_err, DBManager::ErrorPrefix::BIND_ERROR);
            }
        }
        return 0;
    }

    std::unique_ptr<DBManager> DBManager::_manager{nullptr};
    std::filesystem::path DBManager::_db_file_path{util::getDataPath("db.sqlite")};

    DatabaseTable Tables::Status{{"id", "label"}, "status"};
    DatabaseTable Tables::Task{
        {
            "id"
            "parent_id",
            "name",
            "detail",
            "status_id",
            "created_at",
            "updated_at"
        },
        "task"
    };
    DatabaseTable Tables::Worktime{
        {
            "id",
            "task_id",
            "memo",
            "starting_time",
            "finishing_time",
            "created_at",
            "updated_at"
        },
        "worktime"
    };
    DatabaseTable Tables::Schedule{
        {
            "id",
            "task_id",
            "starting_time",
            "finishing_time",
            "created_at",
            "updated_at"
        },
        "schedule"
    };
} // core
