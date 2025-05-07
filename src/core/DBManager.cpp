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
#include <regex>

#include "Logger.h"
#include "../resource.h"

#include "../utilities/Utilities.h"

namespace core::db {
    void sqliteDeleter::DatabaseCloser::operator()(sqlite3* db_) const
    {
        if (db_ != nullptr)
            sqlite3_close_v2(db_);
    }

    void sqliteDeleter::StatementFinalizer::operator()(sqlite3_stmt* stmt_) const
    {
        if (stmt_ != nullptr)
            sqlite3_finalize(stmt_);
    }

    void sqliteDeleter::SqliteStringDeleter::operator()(char* string_) const
    {
        if (string_ != nullptr)
            sqlite3_free(string_);
    }

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

    bool DBManager::setDBFile(const std::string& file_path_)
    {
        std::error_code ec;
        std::filesystem::path tmp_path = std::filesystem::absolute(file_path_, ec);
        if (ec)
            return false;
        _db_file_path = std::move(tmp_path);
        if (_manager != nullptr)
            _manager->_closeDB();
        return true;
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

    int DBManager::execute(const std::string& sql_)
    {
        if (const int open_db_err = openDB(); open_db_err != 0) { return open_db_err; }
        return _manager->_execute(sql_);
    }


    int DBManager::usePlaceholderUniSql(const std::string& sql_, Table& result_table_,
                                        int (*binder_)(void*, sqlite3_stmt*), void* binder_arg_,
                                        std::string& sql_remaining_)
    {
        // db接続を開く(既に開かれている場合は何も実行されない)
        if (const int open_db_err = openDB(); open_db_err != 0) { return open_db_err; }
        return _manager->_usePlaceholderUniSql(sql_, result_table_, binder_, binder_arg_, sql_remaining_);
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
        if (_manager != nullptr && _manager->_db != nullptr) { _manager->_closeDB(); }
        std::error_code remove_err;
        if (std::filesystem::remove(_db_file_path, remove_err)) return openDB();
        return remove_err.value();
    }

    bool DBManager::_isValidErrorPos(const int error_pref_)
    {
        return error_pref_ < static_cast<int>(ErrorPrefix::LAST_ENUM) && error_pref_ >= 0;
    }

    int DBManager::_execute(const std::string& sql_)
    {
        std::lock_guard lock(this->_interface_mtx);
        Table tbl;
        // 残りのsql文
        std::string current_sql = sql_;
        // sql文を全て実行する。
        while (!current_sql.empty()) {
            if (const int err = this->_usePlaceholderUniSqlInternal(current_sql, tbl, nullptr, nullptr, current_sql);
                err != 0) {
                // 最後の文に到達した場合は処理を終了する。
                if (getErrorPos(err) == ErrorPrefix::END_OF_STATEMENT)
                    break;
                return err;
            }
        }
        return 0;
    }

    int DBManager::_usePlaceholderUniSql(const std::string& sql_, Table& result_table_,
                                         int (*binder_)(void*, sqlite3_stmt*), void* binder_arg_,
                                         std::string& sql_remaining_)
    {
        std::lock_guard lock(this->_interface_mtx);
        return _usePlaceholderUniSqlInternal(sql_, result_table_, binder_, binder_arg_, sql_remaining_);
    }

    // ReSharper disable once CppPassValueParameterByConstReference
    int DBManager::_usePlaceholderUniSqlInternal(std::string sql_ /* c_str()による未定義動作を回避するためにコピー */, Table& result_table_, int (*binder_)(void*, sqlite3_stmt*), // NOLINT(performance-unnecessary-value-param)
                                                 void* binder_arg_, std::string& sql_remaining_)
    {
        std::lock_guard lock(this->_internal_mtx);
        if (_db == nullptr) { return getPrefixedErrorCode(0, ErrorPrefix::DB_NOT_OPEN); }
        sqlite3_stmt* tmp_stmt;
        const char* tmp;
        // sqlを準備する。
        if (const int prepare_err = sqlite3_prepare_v2(this->_db.get(), sql_.c_str(), -1,
                                                       &tmp_stmt, &tmp);
            prepare_err != SQLITE_OK) { return getPrefixedErrorCode(prepare_err, ErrorPrefix::PREPARE_SQL_ERROR); }
        // sqlが準備できなかった場合には終了する。(正常終了)
        if (tmp_stmt == nullptr) { return getPrefixedErrorCode(0, ErrorPrefix::END_OF_STATEMENT); }
        sql_remaining_ = std::string(tmp);
        const std::unique_ptr<sqlite3_stmt, sqliteDeleter::StatementFinalizer> stmt(
            tmp_stmt, sqliteDeleter::StatementFinalizer());
        if (binder_ != nullptr) {
            // placeholderと値を紐づける。
            if (const int binder_err = binder_(binder_arg_, stmt.get()); binder_err != SQLITE_OK) { return binder_err; }
        }
        const auto tmp_query_str_c_style = sqlite3ExpandedSqlWrapper(stmt.get());
        const std::string current_query_string = tmp_query_str_c_style.get();
        const auto start_query_at = std::chrono::high_resolution_clock::now();
        const int before_changes = sqlite3_total_changes(_db.get());
        // sqlを実行
        int step_status = sqlite3_step(stmt.get());
        // エラーが発生しておらず、select文であれば、データをクリアする。
        if (sqlite3_stmt_readonly(stmt.get()) && (step_status == SQLITE_ROW || step_status == SQLITE_DONE)) {
            result_table_.clear();
        }
        if (step_status != SQLITE_ROW) {
            if (step_status != SQLITE_DONE) {
                _queryLogger(start_query_at, current_query_string, false, false, 0);
                return getPrefixedErrorCode(step_status, ErrorPrefix::STEP_ERROR);
            }
            const int after_changes = sqlite3_total_changes(_db.get());
            _queryLogger(start_query_at, current_query_string, true, false, after_changes - before_changes);
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

        _queryLogger(start_query_at, current_query_string, true, true, result_table_.size());
        return SQLITE_OK;
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
            if (const int execute_err = _execute(std::string(F_OPEN_DB_PREPROC_SQL, SIZE_OPEN_DB_PREPROC_SQL));
                execute_err != SQLITE_OK) { return getPrefixedErrorCode(execute_err, ErrorPrefix::EXECUTE_ERROR); }
        }
        return SQLITE_OK;
    }

    void DBManager::_closeDB()
    {
        std::scoped_lock lock{this->_interface_mtx, this->_internal_mtx};
        this->_db = nullptr;
    }

    int DBManager::_initializeDB()
    {
        return _execute(
            std::string(F_INITIALIZE_DB_SQL, SIZE_INITIALIZE_DB_SQL)
        );
    }

    const std::regex front_gap_pattern{"^\\s+"};

    void DBManager::_queryLogger(const std::chrono::time_point<std::chrono::high_resolution_clock> start_query_at_,
                                 const std::string& sql_, const bool success_, const bool is_selected,
                                 const int rows_count_)
    {
        const auto end_query_at = std::chrono::high_resolution_clock::now();
        std::string normalized_sql = std::regex_replace(sql_, front_gap_pattern, "");
        std::string summary{};
        if (is_selected || rows_count_ > 0) {
            summary = std::format("{} {} rows - ", is_selected ? "Selected" : "Affected", rows_count_);
        }
        Logger::debug(
            std::format(
                "query:\n{}\n({} ms) {}{}.", normalized_sql,
                std::chrono::duration<double, std::milli>(end_query_at - start_query_at_).count(),
                summary,
                success_ ? "ok" : "failed"
            ),
            "DBManager");
    }

    std::unique_ptr<char, sqliteDeleter::SqliteStringDeleter> DBManager::sqlite3ExpandedSqlWrapper(sqlite3_stmt* stmt_)
    {
        char* value = sqlite3_expanded_sql(stmt_);
        return {value, sqliteDeleter::SqliteStringDeleter()};
    }

    DatabaseTable::DatabaseTable(std::vector<std::string> column_names_, std::string table_name_):
        _column_names(std::move(column_names_)),
        _table_name(std::move(table_name_))
    {
    }

    int DatabaseTable::usePlaceholderUniSql(const std::string& sql_, std::vector<ColValue> placeholder_value_,
                                            std::string& sql_remaining_)
    {
        return DBManager::usePlaceholderUniSql(sql_, _data, _binder, &placeholder_value_, sql_remaining_);
    }

    int DatabaseTable::selectRecords() { return selectRecords("", {}); }

    int DatabaseTable::selectRecords(const std::string& where_clause_, const std::vector<ColValue>& placeholder_value_,
                                     const std::string& order_by_)
    {
        return selectRecords(where_clause_, placeholder_value_, order_by_, -1, -1);
    }

    int DatabaseTable::selectRecords(const std::string& where_clause_, const std::vector<ColValue>& placeholder_value_,
                                     const std::string& order_by_, const int limit_, const int offset_)
    {
        std::string columns;
        for (const std::string& col : _column_names) {
            if (!columns.empty())
                columns += ", ";
            columns += col;
        }
        std::string sql;
        if (where_clause_.empty())
            sql = std::format("SELECT {} FROM {}", columns, _table_name);
        else sql = std::format("SELECT {} FROM {} WHERE {}", columns, _table_name, where_clause_);
        if (order_by_.empty()) sql = std::format("{} ORDER BY {}", sql, order_by_);
        if (limit_ >= 0 && offset_ >= 0) sql = std::format("{} LIMIT {} OFFSET {}", sql, limit_, offset_);
        sql += ";";
        std::string unused_string;
        const int ret_val = usePlaceholderUniSql(sql, placeholder_value_, unused_string);
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

    const std::vector<long long>& TaskTable::getKeys() { return _keys; }

    void TaskTable::_mapper()
    {
        _keys.clear();
        _table.clear();
        for (auto i : _data) {
            _keys.emplace_back(getLongLong(i.at("id")));
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

    std::vector<long long>& ScheduleTable::getKeys() { return _keys; }

    void ScheduleTable::_mapper()
    {
        _keys.clear();
        _table.clear();
        for (auto i : _data) {
            _keys.emplace_back(getLongLong(i.at("id")));
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

    std::vector<long long>& WorktimeTable::getKeys() { return _keys; }

    void WorktimeTable::_mapper()
    {
        _keys.clear();
        _table.clear();
        for (auto i : _data) {
            _keys.emplace_back(getLongLong(i.at("id")));
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
