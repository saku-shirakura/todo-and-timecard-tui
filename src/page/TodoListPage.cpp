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
* @file TodoListPage.cpp
* @date 25/05/01
* @brief 簡単な説明(不要なら削除すること。)
* @details 詳細(不要なら削除すること。)
* @author saku shirakura (saku@sakushira.com)
* @since 
*/

#include "TodoListPage.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include "../core/DBManager.h"
#include "../lib/git-tui/scroller.hpp"

namespace pages {
    void TodoListPage::show()
    {
        auto toggle = ftxui::MenuOption::Toggle();
        toggle.on_change = [&] { resetTaskList(); };
        ftxui::Component task_filter_toggle = ftxui::Menu(&TASK_FILTER_MODE, &_current_task_filter_selected, toggle);
        const ftxui::Component task_filter_container = ftxui::Container::Vertical({task_filter_toggle});
        const ftxui::Component task_list = ftxui::Menu(&_task_labels, &_selected_task, {
                                                           .on_change = [&] {
                                                               if (
                                                                   const size_t task_count = _task_items.getKeys().
                                                                       size();
                                                                   _selected_task >= task_count
                                                               ) {
                                                                   if (task_count == 0) { _selected_task = 0; }
                                                                   else {
                                                                       _selected_task = static_cast<int>(task_count) -
                                                                           1;
                                                                   }
                                                               }
                                                           },
                                                           .on_enter = [&] {
                                                               if (const auto keys = _task_items.getKeys();
                                                                   _selected_task < keys.size()) {
                                                                   _current_parent_id = _task_items.getTable().at(
                                                                       keys.at(_selected_task)).id;
                                                                   resetTaskList();
                                                               }
                                                           },
                                                       });
        task_filter_container->Add(task_list);

        const ftxui::Component console = ftxui::Scroller(
            ftxui::Renderer([&] { return ftxui::paragraph(this->_console_text); })
        );

        task_filter_container->Add(console);

        _renderer = Renderer(task_filter_container, [&] {
            return vbox(
                vbox(
                    hbox(
                        task_filter_toggle->Render(),
                        ftxui::separator()
                    ),
                    ftxui::separator(),
                    vbox(task_list->Render())
                ) | ftxui::border,
                window(ftxui::text("console"),
                       console->Render()
                )
            );
        });

        resetTaskList();
        _render();
    }

    void TodoListPage::updateTaskList()
    {
        std::vector<core::db::ColValue> placeholder_values{};
        std::string where_clause{};
        if (_current_task_filter_selected != 0) {
            where_clause = "status_id=? AND ";
            placeholder_values.emplace_back(core::db::ColType::T_INTEGER, _filterSelectedStatusId());
        }
        if (_current_parent_id == 0) { where_clause += "parent_id IS NULL"; }
        else {
            where_clause += "parent_id=?";
            placeholder_values.emplace_back(core::db::ColType::T_INTEGER, _current_parent_id);
        }
        const int select_err = this->_task_items.selectRecords(where_clause, placeholder_values, "id ASC",
                                                               _tasks_per_page,
                                                               (_task_page - 1) * _tasks_per_page);
        if (select_err != 0) {
            printConsole("Could not retrieve data.");
            return;
        }
        _task_labels.clear();
        _task_labels.resize(_tasks_per_page, "");
        const auto keys = _task_items.getKeys();
        for (size_t i = 0; i < keys.size(); i++) { _task_labels.at(i) = _task_items.getTable().at(keys.at(i)).name; }
    }

    void TodoListPage::resetTaskList()
    {
        _selected_task = 0;
        _task_page = 0;
        nextTaskList();
    }

    void TodoListPage::nextTaskList()
    {
        _updateTaskItemCount();
        if (_task_page == 0 || _tasks_count >= _tasks_per_page * (_task_page + 1))
            _task_page++;
        else return;
        updateTaskList();
    }

    void TodoListPage::prevTaskList()
    {
        if (_task_page > 1) { _task_page--; }
        updateTaskList();
    }

    void TodoListPage::_updateTaskItemCount()
    {
        core::db::Table tmp_tbl;
        std::string unused_str;
        std::string counter_sql{"SELECT COUNT(id) AS rows_count FROM task WHERE "};
        if (_current_parent_id == 0) { counter_sql += "parent_id IS ? "; }
        else { counter_sql += "parent_id=?"; }
        if (_current_task_filter_selected != 0) { counter_sql += " AND status_id=?"; }
        counter_sql += ";";
        const int counter_error = core::db::DBManager::usePlaceholderUniSql(
            counter_sql, tmp_tbl, [](void* this_, sqlite3_stmt* stmt_) {
                const auto this_page = static_cast<TodoListPage*>(this_);
                if (this_page->_current_parent_id == 0) {
                    if (const int bind_err = sqlite3_bind_null(stmt_, 1);
                        bind_err != 0)
                        return bind_err;
                }
                else {
                    if (const int bind_err = sqlite3_bind_int64(
                        stmt_, 1, this_page->_current_parent_id
                    ); bind_err != 0)
                        return bind_err;
                }
                if (this_page->_current_task_filter_selected != 0) {
                    sqlite3_bind_int(
                        stmt_,
                        2,
                        this_page->_filterSelectedStatusId()
                    );
                }
                return 0;
            }, this, unused_str);
        if (counter_error != 0) {
            printConsole("Error: Failed to count tasks.");
            return;
        }
        if (tmp_tbl.front().at("rows_count").first != core::db::ColType::T_INTEGER) {
            printConsole("Error: An invalid type was returned when counting tasks.");
            return;
        }
        _tasks_count = std::get<long long>(tmp_tbl.front().at("rows_count").second);
    }

    bool TodoListPage::_eventHandler(ftxui::Event event_)
    {
        if (event_.is_mouse()) {
            if (const ftxui::Mouse::Button button = event_.mouse().button;
                button == ftxui::Mouse::WheelUp) { this->nextTaskList(); }
            else if (button == ftxui::Mouse::WheelDown) { this->prevTaskList(); }
        }
        else {
            if (event_ == ftxui::Event::ArrowUp && _selected_task >= _tasks_per_page - 1) { this->nextTaskList(); }
            else if (event_ == ftxui::Event::ArrowDown && _selected_task <= 0) { this->prevTaskList(); }
        }
        return true;
    }

    int TodoListPage::_filterSelectedStatusId() const
    {
        return static_cast<int>(FILTER_LABEL_STATUS_MAP.at(
                TASK_FILTER_MODE.at(
                    _current_task_filter_selected
                )
            )
        );
    }

    void TodoListPage::printConsole(const std::string& str_)
    {
        _console_history.push_back(str_);
        if (_console_history.size() > 20) { _console_history.erase(_console_history.begin()); }
        _console_text = "";
        for (size_t i = _console_history.size(); i > 0; i--) {
            _console_text += _console_history.at(i - 1);
            if (i != 1)
                _console_text += "\n";
        }
    }


    const std::vector<std::string> TodoListPage::TASK_FILTER_MODE{
        "All", "In progress", "Incompleted", "Completed", "Not planned"
    };

    const std::unordered_map<std::string, core::db::Status> TodoListPage::FILTER_LABEL_STATUS_MAP{
        {"In progress", core::db::Status::Progress}, {"Incompleted", core::db::Status::Incomplete},
        {"Completed", core::db::Status::Complete}, {"Not planned", core::db::Status::Not_Planned}
    };
} // pages
