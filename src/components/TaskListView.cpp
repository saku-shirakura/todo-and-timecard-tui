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

#include "TaskListView.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>

#include "custom_menu_entry.h"
#include "../core/DBManager.h"
#include "../core/Logger.h"
#include "../core/TodoAndTimeCardApp.h"
#include "../page/TodoListPage.h"
#include "../utilities/DurationTimer.h"

namespace components {
    const std::vector<std::string> TaskListViewData::TASK_FILTER_MODE{
        " All ", " In progress ", " Incompleted ", " Completed ", " Not planned "
    };

    TaskListViewData::TaskListViewData(
        const std::function<void(const std::string& msg_)>& on_error_,
        const std::function<void()>& task_on_selected_)
        : _on_error(on_error_), _task_on_selected(task_on_selected_
                                                      ? task_on_selected_
                                                      : [] {
                                                      })
    {
        _task_items = std::make_shared<core::db::TaskTable>(core::db::TaskTable());
        resetPage();
    }

    void TaskListViewData::updateTaskList()
    {
        // 親タスク名と子タスクを取得する。
        switch (
            const auto [selelct_err, parent_name] = _task_items->fetchChildTasks(
                _parent_id, _status_filter, _page, per_page);
            selelct_err) {
        case 1:
            _on_error("Could not retrieve parent name.");
            break;
        case 2:
            _on_error("Could not retrieve data.");
            break;
        default:
            _parent_name = util::ellipsisString(parent_name, 55);
            break;
        }

        // ラベルを登録する。
        _task_labels->clear();
        _task_labels->resize(per_page, "");
        const auto keys = _task_items->getKeys();
        for (size_t i = 0; i < keys.size(); i++) {
            _task_labels->at(i) = util::ellipsisString(_task_items->getTable().at(keys.at(i)).name, 57);
        }

        // フォーカスを先頭に戻す。
        _selected_task = 0;
        _focused_task = 0;
    }

    long long TaskListViewData::getSelectedTaskId() const
    {
        const auto keys = _task_items->getKeys();
        if (_selected_task < keys.size()) {
            return _task_items->getTable().at(_task_items->getKeys().at(_selected_task)).id;
        }
        return -1;
    }

    std::string TaskListViewData::getSelectedTaskName() const
    {
        const auto keys = _task_items->getKeys();
        if (_selected_task < keys.size()) {
            return _task_items->getTable().at(_task_items->getKeys().at(_selected_task)).name;
        }
        return "";
    }

    long long TaskListViewData::getSelectedTaskStatus() const
    {
        const auto keys = _task_items->getKeys();
        if (_selected_task < keys.size()) {
            return _task_items->getTable().at(_task_items->getKeys().at(_selected_task)).status_id;
        }
        return -1;
    }

    std::string TaskListViewData::getSelectedTaskDetail() const
    {
        const auto keys = _task_items->getKeys();
        if (_selected_task < keys.size()) {
            return _task_items->getTable().at(_task_items->getKeys().at(_selected_task)).detail;
        }
        return "";
    }

    void TaskListViewData::resetPage()
    {
        _page = 0;
        nextPage();
    }

    void TaskListViewData::updatePageCount()
    {
        // 対象となる行数を取得
        auto [count_err, count] = core::db::TaskTable::countChildTasks(
            _parent_id, _status_filter
        );
        if (count_err != 0) {
            _on_error("Failed to count children.");
            return;
        }
        _tasks_count = count;
    }

    void TaskListViewData::nextPage()
    {
        updatePageCount();
        // ページが範囲外にならないように、インクリメントする。
        if (_page == 0 || isExistNextPage())
            _page++;
        else return;

        // ページが変わった場合、データを更新する。
        updateTaskList();
    }

    void TaskListViewData::prevPage()
    {
        // ページが範囲外にならないように、デクリメントする。
        if (isExistPrevPage()) { _page--; }
        else return;

        // ページが変わった場合、データを更新する。
        updateTaskList();
    }

    void TaskListViewData::scrollUpPrevPage()
    {
        if (!isExistPrevPage()) return;
        prevPage();
        if (const size_t task_count = _task_items->getKeys().size();
            task_count != 0) {
            _focused_task = static_cast<int>(task_count) - 1;
            _selected_task = static_cast<int>(task_count) - 1;
        }
    }

    void TaskListViewData::taskListOnChange()
    {
        // 選択されているタスクが、存在しているタスクの数より多い場合
        if (const size_t task_count = _task_items->getKeys().size();
            _selected_task >= task_count) {
            // タスクがないなら、0を常に選択する。
            // タスクがあるなら、最後のタスクを選択し、次のコンポーネントへ移る。
            if (task_count == 0) {
                _selected_task = 0;
                _focused_task = 0;
            }
            else {
                _selected_task = static_cast<int>(task_count) - 1;
                _focused_task = static_cast<int>(task_count) - 1;
            }
        }
    }

    void TaskListViewData::taskListOnEnter()
    {
        const long long id = getSelectedTaskId();
        if (id <= 0) return;
        _parent_id = id;
        resetPage();
    }

    void TaskListViewData::selectTask(const long long task_id_)
    {
        if (task_id_ <= 0) return;
        const auto [err, val] = core::db::TaskTable::fetchPageNumAndFocusFromTask(
            task_id_, _status_filter, per_page);
        if (err != 0) {
            _on_error("Failed to get current task.");
            return;
        }
        const auto [page_num, page_pos] = val;
        _page = static_cast<int>(page_num);
        auto [fetch_parent_err, task] = core::db::TaskTable::fetchTask(task_id_);
        if (fetch_parent_err != 0) {
            _on_error("Failed to get current task record.");
            return;
        }
        _parent_id = task.parent_id;
        updatePageCount();
        updateTaskList();
        _focused_task = static_cast<int>(page_pos);
        _selected_task = static_cast<int>(page_pos);
        _task_on_selected();
    }

    int* TaskListViewData::getSelectedStatusFilter() { return &_status_filter; }

    int* TaskListViewData::getFocusedTaskPtr() { return &_focused_task; }

    int* TaskListViewData::getSelectedTaskPtr() { return &_selected_task; }

    const std::shared_ptr<std::vector<std::string>>& TaskListViewData::getTaskLabels() { return _task_labels; }

    const std::shared_ptr<core::db::TaskTable>& TaskListViewData::getItems() const { return _task_items; }

    bool TaskListViewData::isExistNextPage() const { return _tasks_count > per_page * _page; }

    bool TaskListViewData::isExistPrevPage() const { return _page > 1; }

    int TaskListViewData::getCurrentPage() const { return _page; }

    std::string TaskListViewData::getParentName() { return _parent_name; }

    long long TaskListViewData::getParentId() const { return _parent_id; }

    long long TaskListViewData::getTaskCount() const { return _tasks_count; }

    void TaskListViewData::parentHistoryBack()
    {
        setStatusFilter(0);
        selectTask(_parent_id);
    }

    std::string TaskListViewData::formattedCurrentPage() const
    {
        std::stringstream sstr;
        sstr << std::setw(5) << std::setfill('0') << _page;
        return sstr.str();
    }

    void TaskListViewData::setStatusFilter(const int i)
    {
        if (i > 4 || i <= 0) { _status_filter = 0; }
        else { _status_filter = i; }
    }

    TaskListViewBase::TaskListViewBase(const std::function<void(const std::string& msg_)>& on_error_): _data(on_error_,
        [&] { _task_detail->selectedTaskChanged(); })
    {
        // トップメニュー
        _history_back_button = HistoryBackButton();
        _new_task_button = NewTaskButton();

        const auto current_task_menu = ftxui::Container::Horizontal({});
        current_task_menu->Add(_history_back_button);
        current_task_menu->Add(_new_task_button);

        _status_filter_toggle = StatusFilterToggleMenu();

        // タスクリスト
        _task_list_menu = TaskListMenu();

        // タスク操作・詳細画面
        _task_detail = TaskDetail();

        // パジネーションメニュー
        _pagination_button = ftxui::Container::Horizontal({});
        _prev_button = PrevButton();
        _next_button = NextButton();
        _pagination_button->Add(_prev_button);
        _pagination_button->Add(_next_button);

        const auto task_list_area = ftxui::Container::Vertical({});
        task_list_area->Add(_status_filter_toggle);
        task_list_area->Add(_task_list_menu);

        const auto task_area = ftxui::Container::Horizontal({});
        task_area->Add(task_list_area);
        task_area->Add(_task_detail);

        // コンポーネントを設定
        _main_component = ftxui::Container::Vertical({});
        _main_component->Add(current_task_menu);
        _main_component->Add(task_area);
        _main_component->Add(_pagination_button);
        Add(_main_component);
    }

    ftxui::Element TaskListViewBase::OnRender()
    {
        return
            hbox(
                vbox(
                    hbox(
                        _history_back_button->Render(),
                        ftxui::text(" "),
                        ftxui::text(_data.getParentName()),
                        ftxui::filler(),
                        _new_task_button->Render()
                    ),
                    ftxui::separator(),
                    _status_filter_toggle->Render(),
                    ftxui::separator(),
                    _task_list_menu->Render() | ftxui::reflect(_task_list_box),
                    ftxui::separator(),
                    ftxui::hcenter(
                        ftxui::hbox(
                            _prev_button->Render(),
                            ftxui::hcenter(
                                ftxui::text(
                                    _data.formattedCurrentPage()
                                )
                            ),
                            _next_button->Render()
                        )
                    )
                ),
                ftxui::separator(),
                _task_detail->Render()
            ) | ftxui::border;
    }

    ftxui::Component TaskListViewBase::HistoryBackButton()
    {
        ftxui::ButtonOption button_option = ftxui::ButtonOption::Ascii();

        button_option.label = "↑";
        button_option.on_click = [&] { _data.parentHistoryBack(); };

        return ftxui::Button(button_option);
    }

    ftxui::Component TaskListViewBase::NewTaskButton()
    {
        ftxui::ButtonOption button_option = ftxui::ButtonOption::Ascii();

        button_option.label = "+";
        button_option.on_click = [&] {
            const auto parent_id = _data.getParentId();
            const auto insert_err = core::db::TaskTable::newTask(parent_id);
            if (insert_err != 0) return;
            auto [select_err, task] = core::db::TaskTable::fetchLastTask(_data.getParentId());
            if (select_err != 0) return;
            _data.setStatusFilter(0);
            _data.selectTask(task.id);
        };

        return ftxui::Button(button_option);
    }

    ftxui::Component TaskListViewBase::StatusFilterToggleMenu()
    {
        auto toggle = ftxui::MenuOption::Toggle();
        toggle.on_change = [&] {
            _data.resetPage();
            _task_detail->selectedTaskChanged();
        };
        return ftxui::Menu(&TaskListViewData::TASK_FILTER_MODE, _data.getSelectedStatusFilter(), toggle);
    }

    ftxui::Component TaskListViewBase::TaskListMenu()
    {
        ftxui::MenuOption task_list_option{};
        task_list_option.entries = _data.getTaskLabels().get();
        task_list_option.selected = _data.getSelectedTaskPtr();
        task_list_option.focused_entry = _data.getFocusedTaskPtr();
        task_list_option.on_change = [&] {
            _data.taskListOnChange();
            _task_detail->selectedTaskChanged();
        };
        task_list_option.on_enter = [&] {
            _data.taskListOnEnter();
            _task_detail->selectedTaskChanged();
        };
        task_list_option.entries_option.transform = [&](const ftxui::EntryState& state) {
            const auto keys = _data.getItems()->getKeys();
            if (_data.getTaskCount() <= 0 || state.index >= keys.size())
                return customize::TodoListMenuEntryOptionTransform(state, core::db::Status::Not_Planned, true);

            const auto status = static_cast<core::db::Status>(
                _data.getItems()->getTable().at(keys.at(state.index)).status_id
            );
            return customize::TodoListMenuEntryOptionTransform(state, status, false);
        };

        const auto menu = ftxui::Menu(task_list_option);

        return ftxui::CatchEvent(menu, [&](ftxui::Event event_) {
            if (event_.is_mouse()) {
                if (const auto mouse = event_.mouse(); mouse.button ==
                    ftxui::Mouse::Button::WheelUp) {
                    if (*_data.getSelectedTaskPtr() <= 0 &&
                        _task_list_box.Contain(mouse.x, mouse.y)) {
                        _data.scrollUpPrevPage();
                        return true;
                    }
                }
                else if (mouse.button == ftxui::Mouse::Button::WheelDown) {
                    if (*_data.getSelectedTaskPtr() >= _data.per_page - 1 &&
                        _task_list_box.Contain(mouse.x, mouse.y)) {
                        _data.nextPage();
                        return true;
                    }
                }
                else if (mouse.motion == ftxui::Mouse::Pressed && mouse.button ==
                    ftxui::Mouse::Left) {
                    if (*_data.getFocusedTaskPtr() == *_data.getSelectedTaskPtr() &&
                        _task_list_box.Contain(mouse.x, mouse.y)) {
                        _data.taskListOnEnter();
                        _task_detail->selectedTaskChanged();
                        return true;
                    }
                }
            }
            else if (event_ == ftxui::Event::ArrowDown) {
                if (*_data.getSelectedTaskPtr() >= static_cast<int>(_data.getItems()->
                                                                          getKeys().size()) - 1) {
                    _pagination_button->TakeFocus();
                    return true;
                }
            }
            return false;
        });
    }

    ftxui::Component TaskListViewBase::PrevButton()
    {
        ftxui::ButtonOption prev_button_option = ftxui::ButtonOption::Ascii();
        prev_button_option.label = "←";
        prev_button_option.on_click = [&] { _data.prevPage(); };
        return ftxui::Button(prev_button_option);
    }

    ftxui::Component TaskListViewBase::NextButton()
    {
        ftxui::ButtonOption next_button_option = ftxui::ButtonOption::Ascii();
        next_button_option.label = "→";
        next_button_option.on_click = [&] { _data.nextPage(); };
        return ftxui::Button(next_button_option);
    }

    std::shared_ptr<TaskDetailBase> TaskListViewBase::TaskDetail() { return ftxui::Make<TaskDetailBase>(this); }

    ftxui::Component TaskListView(const std::function<void(const std::string& msg_)>& on_error_)
    {
        return ftxui::Make<TaskListViewBase>(on_error_);
    }

    ActiveTask::ActiveTask(const std::function<void()>& on_jump_button_click_): _on_jump_button_click(
        on_jump_button_click_
            ? on_jump_button_click_
            : [] {
            })
    {
        _jump_button = ftxui::Button("jump", _on_jump_button_click, ftxui::ButtonOption::Ascii());
        Add(_jump_button);
        _loadActiveTask();
    }

    ftxui::Element ActiveTask::OnRender()
    {
        using namespace ftxui;
        const auto active_status = isActivated()
                                       ? text("Active(" + getTimerText() + "): "
                                           + util::ellipsisString(getActiveTaskName(), 36))
                                       : text("Active(00m00s):") | dim;
        return hbox(
            active_status,
            filler(),
            isActivated() ? _jump_button->Render() : text("")
        );
    }

    void ActiveTask::activate(const long long task_id_)
    {
        core::db::WorktimeTable::activateTask(task_id_);
        _activate();
    }

    void ActiveTask::deactivate()
    {
        core::db::WorktimeTable::deactivateAllTasks();
        _active_task_name = "";
        _active_task_id = -1;
        _active_timer.stop();
        _active_timer.setUpdateCallback(nullptr);
        _active_timer.setStartEpoch(0);
        _is_activated = false;
    }

    const std::string& ActiveTask::getActiveTaskName() const { return _active_task_name; }

    const std::string& ActiveTask::getTimerText() { return _active_timer.getText(); }

    std::chrono::seconds ActiveTask::getSeconds() const
    {
        using namespace std::chrono_literals;
        return _active_timer.isActive() ? _active_timer.getSeconds() : 0s;
    }

    bool ActiveTask::isActivated() const { return _is_activated; }

    long long ActiveTask::getActiveTaskId() const { return _active_task_id; }

    void ActiveTask::_loadActiveTask() { _activate(); }

    void ActiveTask::_activate()
    {
        core::db::WorktimeTable table;
        core::db::WorktimeTable::ensureOnlyOneActiveTask();
        table.selectActiveTask();
        if (table.getTable().empty() || table.getKeys().empty()) return;
        const auto worktime = table.getTable().at(table.getKeys().front());
        const auto [fetch_task_err, task] = core::db::TaskTable::fetchTask(worktime.task_id);
        _active_task_name = task.name;
        _active_task_id = task.id;
        _active_timer.start();
        _active_timer.setStartEpoch(worktime.starting_time);
        _active_timer.setUpdateCallback(_onTimerUpdated);
        _is_activated = true;
    }

    void ActiveTask::_onTimerUpdated() { core::TodoAndTimeCardApp::updateScreen(); }

    TaskDetailBase::TaskDetailBase(TaskListViewBase* tasklist_view_base_): _tasklist_view_base(tasklist_view_base_)
    {
        selectedTaskChanged();
        _active_task = ftxui::Make<ActiveTask>([&] { _jumpButtonOnClick(); });
        _task_name_input = ftxui::Input(
            &_task_name, "name"
        ) | ftxui::frame | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 1) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 56);

        _task_status_toggle = ftxui::Toggle(&TaskDetailBase::TASK_STATUS, &_selected_status);

        _task_detail_input = ftxui::Input(&_task_detail, "detail", {
                                              .multiline = true
                                          }
        ) | ftxui::frame | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 10) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 56);

        _worktime_summary = ftxui::Renderer([&] {
            const long long id = _tasklist_view_base->_data.getSelectedTaskId();
            if (_active_task->getActiveTaskId() == id)
                return ftxui::text(util::timeTextFromSeconds(_total_worktime + _active_task->getSeconds()));
            return ftxui::text(util::timeTextFromSeconds(_total_worktime));
        });

        _activate_task_button = ftxui::Button("activate", [&] {
            const long long id = this->_tasklist_view_base->_data.
                                       getSelectedTaskId();
            if (id > 0 && _active_task->getActiveTaskId() != id)
                _active_task->activate(id);
        }) | ftxui::Maybe([&] { return this->_isNotCurrentTaskActivated(); });

        _deactivate_task_button = ftxui::Button("deactivate", [&] {
            _active_task->deactivate();
            updateTotalWorktime();
        }) | ftxui::Maybe([&] { return this->_isCurrentTaskActivated(); });

        _update_button = ftxui::Button("update", [&] { _updateTask(); });

        _delete_button = ftxui::Button("delete", [&] { _deleteTask(); }, ftxui::ButtonOption::Ascii()) | ftxui::Maybe(
            [&] { return this->_selected_status == 3; }) | ftxui::color(ftxui::Color::Red);

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
                        _delete_button->Render()
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
        _task_detail = _tasklist_view_base->_data.getSelectedTaskDetail();
        updateTotalWorktime();
    }

    void TaskDetailBase::updateTotalWorktime()
    {
        using namespace std::chrono_literals;
        const auto [err, seconds] = core::db::TaskTable::fetchTotalWorktime(
            _tasklist_view_base->_data.getSelectedTaskId());
        if (err != 0) _total_worktime = 0s;
        else _total_worktime = seconds;
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
        _tasklist_view_base->_data.setStatusFilter(0);
        _tasklist_view_base->_data.selectTask(id);
    }

    void TaskDetailBase::_jumpButtonOnClick() const
    {
        this->_tasklist_view_base->_data.setStatusFilter(0);
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
} // components
