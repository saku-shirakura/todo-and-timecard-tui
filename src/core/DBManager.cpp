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

#include <complex>
#include <format>
#include <fstream>
#include <iosfwd>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include <memory>

#include "../resource.h"

#include "../utilities/Utilities.h"

namespace core::db {
    double getDouble(const ColValue& value_, const double default_value_)
    {
        if (value_.first == ColType::T_REAL) return std::get<double>(value_.second);
        return default_value_;
    }

    long long getLongLong(const ColValue& value_, const long long default_value_)
    {
        if (value_.first == ColType::T_INTEGER) return std::get<long long>(value_.second);
        return default_value_;
    }

    std::string getString(const ColValue& value_, const std::string& default_value_)
    {
        if (value_.first == ColType::T_TEXT) return std::get<std::string>(value_.second);
        return default_value_;
    }

    void DBManager::setDBFile(std::string file_path_)
    {
        _db_file_path = std::move(file_path_);
        _manager->_closeDB();
    }

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
        if (const int execute_err = sqlite3_exec(_manager->_db.get(), sql_.c_str(), callback_, callback_arg, nullptr);
            execute_err != 0) { return getPrefixedErrorCode(execute_err, ErrorPrefix::EXECUTE_ERROR); }
        return 0;
    }

    int DBManager::usePlaceholderUniSql(const std::string& sql_, Table& result_table_,
                                        int (*binder_)(void*, sqlite3_stmt*), void* binder_arg_)
    {
        // db接続を開く(既に開かれている場合は何も実行されない)
        if (const int open_db_err = openDB(); open_db_err != 0) { return open_db_err; }
        sqlite3_stmt* tmp_stmt;
        // sqlを準備する。
        if (const int prepare_err = sqlite3_prepare_v2(_manager->_db.get(), sql_.c_str(), static_cast<int>(sql_.size()),
                                                       &tmp_stmt, nullptr);
            prepare_err != SQLITE_OK) { return getPrefixedErrorCode(prepare_err, ErrorPrefix::PREPARE_SQL_ERROR); }
        const std::unique_ptr<sqlite3_stmt,sqliteDeleter::StatementFinalizer> stmt(tmp_stmt, sqliteDeleter::StatementFinalizer());
        // placeholderと値を紐づける。
        if (const int binder_err = binder_(binder_arg_, stmt.get()); binder_err != SQLITE_OK) { return binder_err; }
        // sqlを実行
        int step_status = sqlite3_step(stmt.get());
        // エラーが発生しておらず、select文であれば、データをクリアする。
        if (sqlite3_stmt_readonly(stmt.get()) && (step_status == SQLITE_ROW || step_status == SQLITE_DONE)) {
            result_table_.clear();
        }
        if (step_status != SQLITE_ROW) {
            if (step_status != SQLITE_DONE) {
                return getPrefixedErrorCode(step_status, ErrorPrefix::STEP_ERROR);
            }
            return SQLITE_OK;
        }
        // 取得した行をTableに格納する。
        while (step_status == SQLITE_ROW) {
            const int col_count = sqlite3_column_count(stmt.get());
            RowHash current_row;
            // それぞれの列を適切な型でRowHashに格納する。
            for (int col = 0; col < col_count; col++) {
                const int col_type = sqlite3_column_type(stmt.get(), col);
                ColValue col_val;
                switch (col_type) {
                case 2:
                    col_val.first = ColType::T_REAL;
                    col_val.second = sqlite3_column_double(stmt.get(), col);
                    break;
                case 1:
                    col_val.first = ColType::T_INTEGER;
                    col_val.second = sqlite3_column_int64(stmt.get(), col);
                    break;
                case 3:
                    {
                        col_val.first = ColType::T_TEXT;
                        // 戻り値がconst unsigned char*なので、const char*に変換する。
                        const auto text = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), col));
                        const int text_size = sqlite3_column_bytes(stmt.get(), col);
                        col_val.second = std::string(text, text_size);
                        break;
                    }
                case 5:
                default:
                    col_val.first = ColType::T_NULL;
                    col_val.second = nullptr;
                    break;
                }
                const std::string column_name = sqlite3_column_name(stmt.get(), col);
                current_row.try_emplace(column_name, col_val);
            }
            // TableにRowHashを追加。
            result_table_.emplace_back(std::move(current_row));
            // sqlを実行
            step_status = sqlite3_step(stmt.get());
        }
        return SQLITE_OK;
    }

    constexpr int prefix_base = 100000;

    int DBManager::getErrorCode(const int error_code_) { return error_code_ % prefix_base; }

    DBManager::ErrorPrefix DBManager::getErrorPos(const int error_code_)
    {
        int err_pref = std::abs(error_code_) / prefix_base;
        if (!_isValidErrorPos(err_pref)) err_pref = 0;
        return static_cast<ErrorPrefix>(err_pref);
    }

    int DBManager::getPrefixedErrorCode(const int error_code_, ErrorPrefix prefix_)
    {
        return error_code_ + static_cast<int>(prefix_) * prefix_base;
    }

    int DBManager::ReinitializeDB()
    {
        if (_manager != nullptr && _manager->_db != nullptr) {
            _manager->_closeDB();
        }
        std::error_code remove_err;
        if (std::filesystem::remove(_db_file_path, remove_err)) return openDB();
        return remove_err.value();
    }

    bool DBManager::_isValidErrorPos(const int error_pref_)
    {
        return error_pref_ < static_cast<int>(ErrorPrefix::LAST_ENUM) && error_pref_ >= 0;
    }

    int DBManager::_openDB(const std::string& db_file_)
    {
        if (this->_db == nullptr) {
            sqlite3* tmp_db;
            if (const int open_db_err = sqlite3_open(db_file_.c_str(), &tmp_db); open_db_err != SQLITE_OK) {
                sqliteDeleter::DatabaseCloser()(tmp_db);
                return getPrefixedErrorCode(open_db_err, ErrorPrefix::OPEN_DB_ERROR);
            }
            this->_db.reset(tmp_db);
            const int execute_err = sqlite3_exec(
                this->_db.get(), std::string(F_OPEN_DB_PREPROC_SQL, SIZE_OPEN_DB_PREPROC_SQL).c_str(), nullptr,
                nullptr, nullptr);
            if (execute_err != SQLITE_OK) { return getPrefixedErrorCode(execute_err, ErrorPrefix::EXECUTE_ERROR); }
        }
        return SQLITE_OK;
    }

    void DBManager::_closeDB() { this->_db = nullptr; }

    int DBManager::_initializeDB() const
    {
        const int sqlite_err = sqlite3_exec(
            this->_db.get(),
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

    int DatabaseTable::execute(const std::string& sql_, std::vector<ColValue> placeholder_value_)
    {
        if (const int err = DBManager::usePlaceholderUniSql(sql_, _data, _binder, &placeholder_value_); err !=
            SQLITE_OK) { return err; }
        return SQLITE_OK;
    }

    int DatabaseTable::selectRecords() { return selectRecords("", {}); }

    int DatabaseTable::selectRecords(const std::string& where_clause_, const std::vector<ColValue>& placeholder_value_)
    {
        std::string columns;
        for (const std::string& col : _column_names) {
            if (!columns.empty())
                columns += ", ";
            columns += col;
        }
        std::string sql;
        if (where_clause_.empty())
            sql = std::format("SELECT {} FROM {};", columns, _table_name);
        else sql = std::format("SELECT {} FROM {} WHERE {};", columns, _table_name, where_clause_);
        const int ret_val = execute(sql, placeholder_value_);
        _mapper();
        return ret_val;
    }

    const Table& DatabaseTable::getRawTable() { return _data; }

    const std::vector<std::string>& DatabaseTable::getColumnNames() { return _column_names; }

    const std::string& DatabaseTable::getTableName() { return _table_name; }

    int DatabaseTable::_binder(void* bind_arg_, sqlite3_stmt* stmt)
    {
        const auto bind_values = static_cast<std::vector<ColValue>*>(bind_arg_);
        int binder_err = 0;
        for (int i = 0; i < bind_values->size(); i++) {
            const int placeholder_i = i + 1;
            switch (auto [column_type, value] = bind_values->at(i); column_type) {
            case ColType::T_REAL:
                binder_err = sqlite3_bind_double(stmt, placeholder_i, std::get<double>(value));
                break;
            case ColType::T_INTEGER:
                binder_err = sqlite3_bind_int64(stmt, placeholder_i, std::get<long long>(value));
                break;
            case ColType::T_TEXT:
                binder_err = sqlite3_bind_text(stmt, placeholder_i, std::get<std::string>(value).c_str(), -1,
                                               SQLITE_TRANSIENT);
                break;
            case ColType::T_NULL:
                binder_err = sqlite3_bind_null(stmt, placeholder_i);
                break;
            }
            if (binder_err != SQLITE_OK) {
                return DBManager::getPrefixedErrorCode(binder_err, DBManager::ErrorPrefix::BIND_ERROR);
            }
        }
        return SQLITE_OK;
    }

    void DatabaseTable::_mapper()
    {
    }

    Status::Status(const long long id_, std::string label_): id(id_), label(std::move(label_))
    {
    }

    StatusTable::StatusTable(): DatabaseTable({"id", "label"}, "status")
    {
    }

    const std::unordered_map<long long, Status>& StatusTable::getTable() { return _table; }

    void StatusTable::_mapper()
    {
        _table.clear();
        for (auto i : _data) {
            _table.try_emplace(getLongLong(i.at("id")),
                               Status({getLongLong(i.at("id")), getString(i.at("label"))}));
        }
    }


    Task::Task(const long long id_, const long long parent_id_, std::string name_, std::string detail_,
               const long long status_id_, std::string created_at_,
               std::string updated_at_): id(id_), parent_id(parent_id_), name(std::move(name_)),
                                         detail(std::move(detail_)), status_id(status_id_),
                                         created_at(std::move(created_at_)), updated_at(std::move(updated_at_))
    {
    }

    TaskTable::TaskTable(): DatabaseTable({
                                              "id",
                                              "parent_id",
                                              "name",
                                              "detail",
                                              "status_id",
                                              "created_at",
                                              "updated_at"
                                          },
                                          "task")
    {
    }

    const std::unordered_map<long long, Task>& TaskTable::getTable() { return _table; }

    void TaskTable::_mapper()
    {
        _table.clear();
        for (auto i : _data) {
            _table.try_emplace(
                getLongLong(i.at("id")),
                Task({
                    getLongLong(i.at("id")),
                    getLongLong(i.at("parent_id")),
                    getString(i.at("name")),
                    getString(i.at("detail")),
                    getLongLong(i.at("status_id")),
                    getString(i.at("created_at")),
                    getString(i.at("updated_at"))
                }));
        }
    }

    Worktime::Worktime(const long long id_, const long long task_id_, std::string memo_, std::string starting_time_,
                       std::string finishing_time_, std::string created_at_,
                       std::string updated_at_): id(id_), task_id(task_id_), memo(std::move(memo_)),
                                                 starting_time(std::move(starting_time_)),
                                                 finishing_time(std::move(finishing_time_)),
                                                 created_at(std::move(created_at_)), updated_at(std::move(updated_at_))
    {
    }

    Schedule::Schedule(const long long id_, const long long task_id_, std::string starting_time_,
                       std::string finishing_time_, std::string created_at_, std::string updated_at_):
        id(id_),
        task_id(task_id_),
        starting_time(std::move(starting_time_)),
        finishing_time(std::move(finishing_time_)),
        created_at(std::move(created_at_)),
        updated_at(std::move(updated_at_))
    {
    }

    ScheduleTable::ScheduleTable(): DatabaseTable({
                                                      "id",
                                                      "task_id",
                                                      "starting_time",
                                                      "finishing_time",
                                                      "created_at",
                                                      "updated_at"
                                                  },
                                                  "schedule")
    {
    }

    const std::unordered_map<long long, Schedule>& ScheduleTable::getTable() { return _table; }

    void ScheduleTable::_mapper()
    {
        _table.clear();
        for (auto i : _data) {
            _table.try_emplace(
                getLongLong(i.at("id")),
                Schedule({
                    getLongLong(i.at("id")),
                    getLongLong(i.at("task_id")),
                    getString(i.at("starting_time")),
                    getString(i.at("finishing_time")),
                    getString(i.at("created_at")),
                    getString(i.at("updated_at"))
                })
            );
        }
    }


    WorktimeTable::WorktimeTable(): DatabaseTable({
                                                      "id",
                                                      "task_id",
                                                      "memo",
                                                      "starting_time",
                                                      "finishing_time",
                                                      "created_at",
                                                      "updated_at"
                                                  },
                                                  "worktime")
    {
    }

    const std::unordered_map<long long, Worktime>& WorktimeTable::getTable() { return _table; }

    void WorktimeTable::_mapper()
    {
        _table.clear();
        for (auto i : _data) {
            _table.try_emplace(
                getLongLong(i.at("id")),
                Worktime({
                    getLongLong(i.at("id")),
                    getLongLong(i.at("task_id")),
                    getString(i.at("memo")),
                    getString(i.at("starting_time")),
                    getString(i.at("finishing_time")),
                    getString(i.at("created_at")),
                    getString(i.at("updated_at"))
                }));
        }
    }

    std::unique_ptr<DBManager> DBManager::_manager{nullptr};
    std::filesystem::path DBManager::_db_file_path{util::getDataPath("db.sqlite")};
} // core
