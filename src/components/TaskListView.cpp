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

#include <stack>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>

#include "custom_menu_entry.h"
#include "../core/DBManager.h"
#include "../core/Logger.h"
#include "../core/TodoAndTimeCardApp.h"
#include "../page/TodoListPage.h"
#include "../utilities/DurationTimer.h"

namespace components {
    class TaskListViewData {
    public:
        explicit TaskListViewData(const std::function<void(const std::string& msg_)>& on_error_): _on_error(on_error_)
        {
            _task_items = std::make_shared<core::db::TaskTable>(core::db::TaskTable());
            resetPage();
        }

        void updateTaskList()
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

        void resetPage()
        {
            _page = 0;
            nextPage();
        }

        void updatePageCount()
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

        void nextPage()
        {
            updatePageCount();
            // ページが範囲外にならないように、インクリメントする。
            if (_page == 0 || isExistNextPage())
                _page++;
            else return;

            // ページが変わった場合、データを更新する。
            updateTaskList();
        }

        void prevPage()
        {
            // ページが範囲外にならないように、デクリメントする。
            if (isExistPrevPage()) { _page--; }
            else return;

            // ページが変わった場合、データを更新する。
            updateTaskList();
        }

        void scrollUpPrevPage()
        {
            if (!isExistPrevPage()) return;
            prevPage();
            if (const size_t task_count = _task_items->getKeys().size();
                task_count != 0) {
                _focused_task = static_cast<int>(task_count) - 1;
                _selected_task = static_cast<int>(task_count) - 1;
            }
        }

        void taskListOnChange()
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

        void taskListOnEnter()
        {
            // 選択されたタスクが範囲内であれば、そのタスクに移動する。
            if (const auto keys = _task_items->getKeys();
                _selected_task < keys.size()) {
                _parent_id = _task_items->getTable().at(_task_items->getKeys().at(_selected_task)).id;
                resetPage();
            }
        }

        void selectTask(const long long task_id_)
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
        }

        int* getSelectedStatusFilter() { return &_status_filter; }

        int* getFocusedTaskPtr() { return &_focused_task; }

        int* getSelectedTaskPtr() { return &_selected_task; }

        [[nodiscard]] const std::shared_ptr<std::vector<std::string>>& getTaskLabels() { return _task_labels; }

        [[nodiscard]] const std::shared_ptr<core::db::TaskTable>& getItems() const { return _task_items; }

        [[nodiscard]] bool isExistNextPage() const { return _tasks_count > per_page * _page; }

        [[nodiscard]] bool isExistPrevPage() const { return _page > 1; }

        [[nodiscard]] int getCurrentPage() const { return _page; }

        std::string getParentName() { return _parent_name; }

        [[nodiscard]] long long getTaskCount() const { return _tasks_count; }

        void parentHistoryBack()
        {
            setStatusFilter(0);
            selectTask(_parent_id);
        }


        [[nodiscard]] std::string formattedCurrentPage() const
        {
            std::stringstream sstr;
            sstr << std::setw(5) << std::setfill('0') << _page;
            return sstr.str();
        }

        void setStatusFilter(const int i)
        {
            if (i > 4 || i <= 0) { _status_filter = 0; }
            else { _status_filter = i; }
        }

        static const std::vector<std::string> TASK_FILTER_MODE;

        const int per_page = 20;

    private:
        // エラーハンドラ
        std::function<void(const std::string& msg_)> _on_error;

        // タスクリスト関係
        std::shared_ptr<std::vector<std::string>> _task_labels{new std::vector<std::string>};
        std::shared_ptr<core::db::TaskTable> _task_items{new core::db::TaskTable};
        int _selected_task = 0;
        int _focused_task = 0;
        long long _tasks_count = 0;

        // ページ関係
        int _page = 0;

        // フィルタ関係
        int _status_filter{0};

        // 親タスク関係
        long long _parent_id{0};
        std::string _parent_name{};
    };

    const std::vector<std::string> TaskListViewData::TASK_FILTER_MODE{
        " All ", " In progress ", " Incompleted ", " Completed ", " Not planned "
    };

    // TODO: タスクの詳細を変更可能にする。(進行状況も含む)
    // TODO: タスクを追加可能にする。
    // TODO: タスクを削除可能にする。
    class TaskListViewBase final : public ftxui::ComponentBase {
    public:
        explicit TaskListViewBase(const std::function<void(const std::string& msg_)>& on_error_): _data(on_error_)
        {
            // トップメニュー
            _history_back_button = HistoryBackButton();
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
            _main_component->Add(_history_back_button);
            _main_component->Add(task_area);
            _main_component->Add(_pagination_button);
            Add(_main_component);
        }

        ftxui::Element OnRender() override
        {
            return
                hbox(
                    vbox(
                        hbox(
                            _history_back_button->Render(),
                            ftxui::text(" "),
                            ftxui::text(_data.getParentName())
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

    private:
        ftxui::Component HistoryBackButton()
        {
            ftxui::ButtonOption button_option = ftxui::ButtonOption::Ascii();

            button_option.label = "↑";
            button_option.on_click = [&] { _data.parentHistoryBack(); };

            return ftxui::Button(button_option);
        }

        ftxui::Component StatusFilterToggleMenu()
        {
            auto toggle = ftxui::MenuOption::Toggle();
            toggle.on_change = [&] { _data.resetPage(); };
            return ftxui::Menu(&TaskListViewData::TASK_FILTER_MODE, _data.getSelectedStatusFilter(), toggle);
        }

        ftxui::Component TaskListMenu()
        {
            ftxui::MenuOption task_list_option{};
            task_list_option.entries = _data.getTaskLabels().get();
            task_list_option.selected = _data.getSelectedTaskPtr();
            task_list_option.focused_entry = _data.getFocusedTaskPtr();
            task_list_option.on_change = [&] { _data.taskListOnChange(); };
            task_list_option.on_enter = [&] { _data.taskListOnEnter(); };
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

        ftxui::Component PrevButton()
        {
            ftxui::ButtonOption prev_button_option = ftxui::ButtonOption::Ascii();
            prev_button_option.label = "←";
            prev_button_option.on_click = [&] { _data.prevPage(); };
            return ftxui::Button(prev_button_option);
        }

        ftxui::Component NextButton()
        {
            ftxui::ButtonOption next_button_option = ftxui::ButtonOption::Ascii();
            next_button_option.label = "→";
            next_button_option.on_click = [&] { _data.nextPage(); };
            return ftxui::Button(next_button_option);
        }

        class TaskDetailBase final : public ComponentBase {
        public:
            explicit TaskDetailBase(TaskListViewBase* tasklist_view_base_): _tasklist_view_base(tasklist_view_base_)
            {
                _jump_button = ftxui::Button("jump", [&] {
                    this->_tasklist_view_base->_data.setStatusFilter(0);
                    this->_tasklist_view_base->_data.selectTask(
                        _active_task.getActiveTaskId());
                }, ftxui::ButtonOption::Ascii());
                const auto main_container = ftxui::Container::Vertical({});
                main_container->Add(_jump_button);
                Add(main_container);
            }

            ftxui::Element OnRender() override
            {
                using namespace ftxui;
                const auto active_status = _active_task.isActivated()
                                               ? text("Active(" + _active_task.getTimerText() + "): "
                                                   + util::ellipsisString(_active_task.getActiveTaskName(), 34))
                                               : text("Inactive.");
                return vbox(
                    hbox(
                        active_status,
                        _active_task.isActivated() ? _jump_button->Render() | align_right : text("")
                    ),
                    separator(),
                    paragraph(
                        "WIP Area of task details.")
                );
            }

        private:
            class ActiveTask {
            public:
                ActiveTask() { _loadActiveTask(); }

                void activate(const long long task_id_)
                {
                    core::db::WorktimeTable::activateTask(task_id_);
                    _activate();
                }

                void deactivate()
                {
                    core::db::WorktimeTable::deactivateAllTasks();
                    _active_timer.setUpdateCallback(nullptr);
                    _active_timer.setStartEpoch(0);
                    _is_activated = false;
                }

                const std::string& getActiveTaskName() { return _active_task_name; }

                const std::string& getTimerText() { return _active_timer.getText(); }

                bool isActivated() const { return _is_activated; }

                long long getActiveTaskId() const { return _active_task_id; }

            private:
                void _loadActiveTask() { _activate(); }

                void _activate()
                {
                    core::db::WorktimeTable table;
                    table.ensureOnlyOneActiveTask();
                    table.selectActiveTask();
                    if (table.getTable().empty() || table.getKeys().empty()) return;
                    const auto worktime = table.getTable().at(table.getKeys().front());
                    const auto [fetch_task_err, task] = core::db::TaskTable::fetchTask(worktime.task_id);
                    _active_task_name = task.name;
                    _active_task_id = task.id;
                    _active_timer.setStartEpoch(worktime.starting_time);
                    _active_timer.setUpdateCallback(_onTimerUpdated);
                    _is_activated = true;
                }

                static void _onTimerUpdated() { core::TodoAndTimeCardApp::updateScreen(); }

                bool _is_activated{false};
                std::string _active_task_name{};
                long long _active_task_id{-1};
                DurationTimer _active_timer{0};
            };

            TaskListViewBase* _tasklist_view_base;
            ActiveTask _active_task;
            ftxui::Component _jump_button;
        };

        ftxui::Component TaskDetail()
        {
            // TODO: WIP 実装する。
            return ftxui::Make<TaskDetailBase>(this);
        }

        TaskListViewData _data;

        ftxui::Component _history_back_button;
        ftxui::Component _status_filter_toggle;
        ftxui::Component _pagination_button;
        ftxui::Component _prev_button;
        ftxui::Component _next_button;

        ftxui::Component _task_list_menu;
        ftxui::Component _task_detail;
        ftxui::Component _main_component;

        ftxui::Box _task_list_box;
    };

    ftxui::Component TaskListView(const std::function<void(const std::string& msg_)>& on_error_)
    {
        return ftxui::Make<TaskListViewBase>(on_error_);
    }
} // components
