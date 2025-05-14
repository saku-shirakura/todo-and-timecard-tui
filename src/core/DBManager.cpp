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
                                 const size_t rows_count_)
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

    int DatabaseTable::usePlaceholderUniSql(const std::string& sql_, std::vector<ColValue> placeholder_value_)
    {
        std::string sql_remaining{};
        return DBManager::usePlaceholderUniSql(sql_, _data, _binder, &placeholder_value_, sql_remaining);
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
        if (!order_by_.empty()) sql = std::format("{} ORDER BY {}", sql, order_by_);
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


    Task::Task(): id(-1), parent_id(-1), status_id(0), created_at(-1), updated_at(-1)
    {
    }

    Task::Task(const long long id_, const long long parent_id_, std::string name_, std::string detail_,
               const long long status_id_, const long long created_at_,
               const long long updated_at_): id(id_), parent_id(parent_id_), name(std::move(name_)),
                                             detail(std::move(detail_)), status_id(status_id_),
                                             created_at(created_at_), updated_at(updated_at_)
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

    const std::unordered_map<long long, Task>& TaskTable::getTable() const { return _table; }

    const std::vector<long long>& TaskTable::getKeys() const { return _keys; }

    std::pair<int, std::string> TaskTable::fetchChildTasks(long long parent_task_id_, const int status_filter_,
                                                           const int page_, const int per_page_)
    {
        std::string parent_task_name;
        std::vector<ColValue> placeholder_values{};
        std::string where_clause{};
        TaskTable select_parent_task_name{};

        // 親タスク名を取得する。
        if (const int err = select_parent_task_name.selectRecords("id=?", {
                                                                      {ColType::T_INTEGER, parent_task_id_}
                                                                  }); err != 0) { return {1, ""}; }
        if (select_parent_task_name.getTable().contains(parent_task_id_)) {
            parent_task_name = select_parent_task_name.getTable().at(parent_task_id_).name;
        }
        else { parent_task_name = ""; }

        // フィルタが有効な場合は設定する。
        if (status_filter_ != 0) {
            where_clause = "status_id=? AND ";
            placeholder_values.emplace_back(ColType::T_INTEGER, status_filter_);
        }

        // parent_id_を指定する。
        if (parent_task_id_ <= 0) { where_clause += "parent_id IS NULL"; }
        else {
            where_clause += "parent_id=?";
            placeholder_values.emplace_back(ColType::T_INTEGER, parent_task_id_);
        }

        // タスクテーブルを最新のparent_idのものに更新する。
        const int select_err = this->selectRecords(where_clause, placeholder_values,
                                                   "status_id, name ASC",
                                                   per_page_,
                                                   (page_ - 1) * per_page_);
        if (select_err != 0) { return {2, parent_task_name}; }
        return {0, parent_task_name};
    }

    std::pair<int, long long> TaskTable::countChildTasks(const long long parent_task_id_, const int filter_status_)
    {
        std::string unuse;
        std::string cond{};

        // 親タスクのIDを指定する。
        if (parent_task_id_ <= 0) { cond = "parent_id IS NULL"; }
        else { cond = "parent_id=" + std::to_string(parent_task_id_); }

        // フィルタが有効な範囲であれば、ステータスIDでフィルタリングする。
        if (filter_status_ > 0 && filter_status_ <= 4) cond += " AND status_id=" + std::to_string(filter_status_);

        TaskTable tmp_table{};
        if (int err = tmp_table.usePlaceholderUniSql(
            std::format("SELECT COUNT(ID) AS task_count FROM task WHERE {};", cond),
            {},
            unuse
        ); err != 0)
            return {err, 0};

        // データが正常に取得できているなら、その値を返す。
        // レコードの存在確認
        if (const auto raw_table = tmp_table.getRawTable(); raw_table.size() > 0) {
            // 列の存在確認
            if (const auto front = raw_table.front(); front.contains("task_count")) {
                // 型チェック
                if (auto [type, value] = front.at("task_count"); type == ColType::T_INTEGER) {
                    return {0, std::get<long long>(value)};
                }
            }
        }
        return {-1, 0};
    }

    std::pair<int, std::pair<long long, long long>> TaskTable::fetchPageNumAndFocusFromTask(
        const long long task_id_, const int status_filter_, const int per_page_)
    {
        std::string unuse;
        std::string sql{};
        TaskTable tmp_tbl;

        // 対象のタスクを取得。
        const auto [fetch_task_err, task] = fetchTask(task_id_);
        if (fetch_task_err != 0) return {-1, {-1, -1}};
        long long parent_task_id = task.parent_id;

        // SQLを動的に組み立てる
        // docs/taskFromPageNumber.sqlに記載
        sql = "SELECT row_id / " + std::to_string(per_page_) + " + 1 AS page_num,";
        sql += " row_id % " + std::to_string(per_page_) + " - 1 AS page_pos";
        sql += " FROM (";
        sql += " SELECT id, row_number() over (ORDER BY status_id, name) AS row_id";
        sql += " FROM task WHERE parent_id";

        // 親タスクIDを設定。0以下であればNULLとみなす。
        if (parent_task_id <= 0) { sql += " IS NULL"; }
        else { sql += "=" + std::to_string(parent_task_id); }

        // 取得対象のステータスIDを設定。範囲外なら設定しない。
        if (status_filter_ > 0 && status_filter_ <= 4) { sql += " AND status_id=" + std::to_string(status_filter_); }

        // タスクIDで絞り込む。
        sql += ") WHERE id=" + std::to_string(task_id_) + ";";

        // SQLを実行。
        if (const auto err = tmp_tbl.usePlaceholderUniSql(sql, {}, unuse);
            err != 0) { return {err, {-1, -1}}; }

        // テーブルに値が存在するか確認
        const auto raw_tbl = tmp_tbl.getRawTable();
        if (raw_tbl.empty()) return {-2, {-1, -1}};
        if (!raw_tbl.front().contains("page_num") || !raw_tbl.front().contains("page_pos")) return {-3, {-1, -1}};

        // ページ番号を取得
        long long page_number = 0;
        const auto [type_num, val_num] = raw_tbl.front().at("page_num");
        if (
            type_num != ColType::T_INTEGER) { return {-4, {-1, -1}}; }
        page_number = std::get<long long>(val_num);

        // ページ内での位置を取得
        long long page_pos = 0;
        const auto [type_pos, val_pos] = raw_tbl.front().at("page_pos");
        if (
            type_pos != ColType::T_INTEGER) { return {-5, {-1, -1}}; }
        page_pos = std::get<long long>(val_pos);
        return {0, {page_number, page_pos}};
    }

    std::pair<int, Task> TaskTable::fetchTask(long long task_id_)
    {
        TaskTable table;
        if (const int err = table.selectRecords("id=?", {{ColType::T_INTEGER, task_id_}}); err != 0) {
            return {err, Task()};
        }
        if (!table.getTable().contains(task_id_)) return {-1, Task()};
        Task task = table.getTable().at(task_id_);
        return {0, task};
    }

    std::pair<int, std::chrono::seconds> TaskTable::fetchTotalWorktime(long long task_id_)
    {
        using namespace std::chrono_literals;
        TaskTable table;
        int err = table.usePlaceholderUniSql(
            std::string(F_SUM_TOTAL_WORKTIME_SQL, SIZE_SUM_TOTAL_WORKTIME_SQL),
            {
                {ColType::T_INTEGER, task_id_}
            }
        );
        if (err != 0) return {err, -1s};
        if (table.getRawTable().empty()) return {-1, -1s};
        if (!table.getRawTable().front().contains("total_worktime")) return {-2, -1s};
        return {0, std::chrono::seconds(getLongLong(table.getRawTable().front().at("total_worktime")))};
    }

    std::pair<int, Task> TaskTable::fetchLastTask(const long long parent_id)
    {
        TaskTable table;
        auto placeholder = std::vector<ColValue>();

        std::string where = "parent_id = ?";
        if (parent_id <= 0) where = "parent_id IS NULL";
        else placeholder.emplace_back(ColType::T_INTEGER, parent_id);

        const int err = table.selectRecords(where, placeholder, "id DESC", 1, 0);

        if (err != 0) return {err, Task()};
        if (table.getKeys().empty() || table.getTable().empty()) return {-1, Task()};
        return {0, table.getTable().at(table.getKeys().front())};
    }

    int TaskTable::newTask(long long parent_id)
    {
        TaskTable table;
        const int err = table.usePlaceholderUniSql(
            "INSERT INTO task(parent_id, name, status_id) VALUES (?, 'New Task', 2);", {
                (parent_id <= 0 ? ColValue(ColType::T_NULL, nullptr) : ColValue(ColType::T_INTEGER, parent_id))
            }
        );
        if (err != 0) return err;
        return 0;
    }

    int TaskTable::deleteTask(const long long task_id)
    {
        TaskTable tbl;
        return tbl.usePlaceholderUniSql(
            "DELETE FROM task WHERE id = ?;",
            {{ColType::T_INTEGER, task_id}}
        );
    }

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
                    getLongLong(i.at("created_at")),
                    getLongLong(i.at("updated_at"))
                }));
        }
    }

    Worktime::Worktime(const long long id_, const long long task_id_, const long long starting_time_,
                       const long long finishing_time_, const long long created_at_,
                       const long long updated_at_): id(id_), task_id(task_id_),
                                                     starting_time(starting_time_),
                                                     finishing_time(finishing_time_),
                                                     created_at(created_at_), updated_at(updated_at_)
    {
    }

    Schedule::Schedule(const long long id_, const long long task_id_, const long long starting_time_,
                       const long long finishing_time_, const long long created_at_, const long long updated_at_):
        id(id_),
        task_id(task_id_),
        starting_time(starting_time_),
        finishing_time(finishing_time_),
        created_at(created_at_),
        updated_at(updated_at_)
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

    const std::unordered_map<long long, Schedule>& ScheduleTable::getTable() const { return _table; }

    const std::vector<long long>& ScheduleTable::getKeys() const { return _keys; }

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
                    getLongLong(i.at("starting_time")),
                    getLongLong(i.at("finishing_time")),
                    getLongLong(i.at("created_at")),
                    getLongLong(i.at("updated_at"))
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

    const std::unordered_map<long long, Worktime>& WorktimeTable::getTable() const { return _table; }

    const std::vector<long long>& WorktimeTable::getKeys() const { return _keys; }

    int WorktimeTable::ensureOnlyOneActiveTask()
    {
        WorktimeTable table;
        return table.usePlaceholderUniSql(std::string(F_CHANGE_TO_ONLY_ONE_TASK_SQL, SIZE_CHANGE_TO_ONLY_ONE_TASK_SQL));
    }

    int WorktimeTable::deactivateAllTasks()
    {
        WorktimeTable table;
        return table.usePlaceholderUniSql(
            "UPDATE worktime SET finishing_time = (strftime('%s', DATETIME('now'))) WHERE finishing_time IS NULL;");
    }

    int WorktimeTable::selectActiveTask()
    {
        const int err = usePlaceholderUniSql(std::string(F_SELECT_ACTIVE_TASK_SQL, SIZE_SELECT_ACTIVE_TASK_SQL));
        if (err != 0) return err;
        _mapper();
        return 0;
    }

    int WorktimeTable::activateTask(const long long task_id_)
    {
        WorktimeTable table;
        deactivateAllTasks();
        return table.usePlaceholderUniSql("INSERT INTO worktime(task_id) VALUES (?);", {
                                              {ColType::T_INTEGER, task_id_}
                                          });
    }

    int WorktimeTable::updateWorktime(long long id_, const std::string& memo_)
    {
        WorktimeTable table;
        return table.usePlaceholderUniSql("UPDATE worktime SET memo=? WHERE id=?;", {
                                              {ColType::T_TEXT, memo_},
                                              {ColType::T_INTEGER, id_}
                                          });
    }

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
                    getLongLong(i.at("starting_time")),
                    getLongLong(i.at("finishing_time")),
                    getLongLong(i.at("created_at")),
                    getLongLong(i.at("updated_at"))
                }));
        }
    }

    std::unique_ptr<DBManager> DBManager::_manager{nullptr};
    std::filesystem::path DBManager::_db_file_path{util::getDataPath("db.sqlite")};
} // core
