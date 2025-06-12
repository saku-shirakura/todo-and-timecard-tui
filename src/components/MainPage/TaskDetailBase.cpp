// MIT License
//
// Copyright (c) 2025 Saku Shirakura <saku@sakushira.com>
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


#include "MainPage.h"

#include <ftxui/component/component.hpp>
#include "../../utilities/Utilities.h"

namespace components {
    TaskDetailBase::TaskDetailBase(TaskListViewBase* tasklist_view_base_): _tasklist_view_base(tasklist_view_base_)
    {
        _active_task = ftxui::Make<ActiveTaskBase>([&] { _jumpButtonOnClick(); });
        _task_name_input = ftxui::Input(
            &_task_name, "name"
        ) | ftxui::frame | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 1) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 56);

        auto taskStatusToggleOption = ftxui::MenuOption::Toggle();
        taskStatusToggleOption.focused_entry = &_focused_status;
        _task_status_toggle = ftxui::Menu(&TaskDetailBase::TASK_STATUS, &_selected_status, taskStatusToggleOption);

        _task_detail_input = ftxui::Input(&_task_detail, "detail", {
                                              .multiline = true
                                          }
        ) | ftxui::frame | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 10) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 56);

        _worktime_summary = ftxui::Renderer([&] {
            std::string str_time{};
            // アクティブタスクが選択中のタスクの子孫または選択中のタスクであれば、作業時間を加算する。
            std::chrono::seconds tmp_total_worktime = _total_worktime;
            if (_is_active_task_family)
                tmp_total_worktime += _active_task->getSeconds();
            str_time = util::timeTextFromSeconds(tmp_total_worktime);
            // 選択中タスク単体の作業時間
            std::chrono::seconds tmp_total_task_worktime = _task_worktime;
            if (_active_task->getActiveTaskId() == this->_tasklist_view_base->_data.getSelectedTaskId())
                tmp_total_task_worktime += _active_task->getSeconds();
            str_time += " (In: " + util::timeTextFromSeconds(tmp_total_task_worktime) + ")";
            return ftxui::text(str_time);
        });

        _activate_task_button = ftxui::Button("activate", [&] {
            const long long id = this->_tasklist_view_base->_data.
                                       getSelectedTaskId();
            if (id > 0 && _active_task->getActiveTaskId() != id) {
                _active_task->activate(id);
                _is_active_task_family = true;
            }
        }) | ftxui::Maybe([&] { return this->_isNotCurrentTaskActivated(); });

        _deactivate_task_button = ftxui::Button("deactivate", [&] {
            _active_task->deactivate();
            updateTotalWorktime();
            updateTaskWorktime();
            _is_active_task_family = false;
        }) | ftxui::Maybe([&] { return this->_isCurrentTaskActivated(); });

        _update_button = ftxui::Button("update", [&] { _updateTask(); });

        _delete_button = ftxui::Button("delete", [&] { _deleteTask(); }, ftxui::ButtonOption::Ascii()) | ftxui::Maybe(
            [&] { return this->_selected_status == 3; }) | ftxui::color(ftxui::Color::Red);

        selectedTaskChanged();

        const auto detail_container = ftxui::Container::Vertical({});
        detail_container->Add(_task_name_input);
        detail_container->Add(_task_status_toggle);
        detail_container->Add(_task_detail_input);
        detail_container->Add(_worktime_summary);
        detail_container->Add(_activate_task_button);
        detail_container->Add(_deactivate_task_button);

        const auto main_container = ftxui::Container::Vertical({});
        main_container->Add(_active_task);
        main_container->Add(detail_container);
        main_container->Add(_update_button);
        main_container->Add(_delete_button);
        Add(main_container);
    }

    ftxui::Element TaskDetailBase::OnRender()
    {
        using namespace ftxui;
        return
            vbox(
                _active_task->Render(),
                separator(),
                _tasklist_view_base->_data.getSelectedTaskStatus() <= 0
                    ? text("")
                    : vbox(
                        _task_name_input->Render(),
                        separator(),
                        hbox(
                            _task_status_toggle->Render(),
                            separator()
                        ),
                        separator(),
                        _task_detail_input->Render(),
                        separator(),
                        _worktime_summary->Render(),
                        separator(),
                        _activate_task_button->Render(),
                        _deactivate_task_button->Render(),
                        _update_button->Render(),
                        hbox(filler(), _delete_button->Render())
                    ) | yframe
            ) | flex_grow;
    }

    void TaskDetailBase::selectedTaskChanged()
    {
        using namespace std::chrono_literals;
        const auto status = _tasklist_view_base->_data.getSelectedTaskStatus();
        if (status <= 0) {
            _task_name = "";
            _selected_status = 0;
            _task_detail = "";
            _total_worktime = 0s;
            return;
        }
        _task_name = _tasklist_view_base->_data.getSelectedTaskName();
        _selected_status = static_cast<int>(status) - 1;
        _focused_status = static_cast<int>(status) - 1;
        _task_detail = _tasklist_view_base->_data.getSelectedTaskDetail();
        updateTotalWorktime();
        updateTaskWorktime();
        const long long id = _tasklist_view_base->_data.getSelectedTaskId();
        _is_active_task_family = core::db::TaskTable::computeIsSiblings(_active_task->getActiveTaskId(), id);
    }

    void TaskDetailBase::updateTotalWorktime()
    {
        using namespace std::chrono_literals;
        const auto [err, seconds] = core::db::TaskTable::computeTotalWorktime(
            _tasklist_view_base->_data.getSelectedTaskId());
        if (err != 0) _total_worktime = 0s;
        else _total_worktime = seconds;
    }

    void TaskDetailBase::updateTaskWorktime()
    {
        using namespace std::chrono_literals;
        const auto [err, seconds] = core::db::TaskTable::fetchWorktime(
            _tasklist_view_base->_data.getSelectedTaskId());
        if (err != 0) _task_worktime = 0s;
        else _task_worktime = seconds;
    }

    void TaskDetailBase::_deleteTask()
    {
        if (_selected_status != 3) return;
        const auto id = this->_tasklist_view_base->_data.getSelectedTaskId();
        if (core::db::TaskTable::deleteTask(id) != 0) return;
        this->_tasklist_view_base->_data.resetPage();
        selectedTaskChanged();
    }

    void TaskDetailBase::_updateTask()
    {
        const long long id = _tasklist_view_base->_data.getSelectedTaskId();
        if (id <= 0) return;
        core::db::TaskTable table;
        const int err = table.usePlaceholderUniSql("UPDATE task SET name = ?, detail = ?, status_id = ? WHERE id = ?", {
                                                       {core::db::ColType::T_TEXT, _task_name},
                                                       {core::db::ColType::T_TEXT, _task_detail},
                                                       {core::db::ColType::T_INTEGER, _selected_status + 1},
                                                       {core::db::ColType::T_INTEGER, id}
                                                   });
        if (err != 0) return;
        _tasklist_view_base->_data.selectTask(id);
    }

    void TaskDetailBase::_jumpButtonOnClick() const
    {
        this->_tasklist_view_base->_data.selectTask(_active_task->getActiveTaskId());
    }

    bool TaskDetailBase::_isNotCurrentTaskActivated() const
    {
        const long long id = this->_tasklist_view_base->_data.getSelectedTaskId();
        return id > 0 && _active_task->getActiveTaskId() != id;
    }

    bool TaskDetailBase::_isCurrentTaskActivated() const
    {
        const long long id = this->_tasklist_view_base->_data.getSelectedTaskId();
        return _active_task->isActivated() && id > 0 && _active_task->getActiveTaskId() == id;
    }

    const std::vector<std::string> TaskDetailBase::TASK_STATUS{
        " In progress ", " Incompleted ", " Completed ", " Not planned "
    };
}
