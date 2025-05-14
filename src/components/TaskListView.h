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

/**
 * @file TaskListView.h
 * @date 25/05/08
 * @brief ファイルの説明
 * @details ファイルの詳細
 * @author saku shirakura (saku@sakushira.com)
 */


#ifndef TASKLISTVIEW_H
#define TASKLISTVIEW_H
#include <ftxui/component/component_base.hpp>

#include "../core/DBManager.h"
#include "../utilities/DurationTimer.h"

namespace components {
    class TaskDetailBase;

    class TaskListViewData {
    public:
        explicit TaskListViewData(
            const std::function<void(const std::string& msg_)>& on_error_,
            const std::function<void()>& task_on_selected_
        );

        void updateTaskList();

        [[nodiscard]] long long getSelectedTaskId() const;

        [[nodiscard]] std::string getSelectedTaskName() const;

        [[nodiscard]] long long getSelectedTaskStatus() const;

        [[nodiscard]] std::string getSelectedTaskDetail() const;

        void resetPage();

        void updatePageCount();

        void nextPage();

        void prevPage();

        void scrollUpPrevPage();

        void taskListOnChange();

        void taskListOnEnter();

        void selectTask(long long task_id_);

        int* getSelectedStatusFilter();

        int* getFocusedTaskPtr();

        int* getSelectedTaskPtr();

        [[nodiscard]] const std::shared_ptr<std::vector<std::string>>& getTaskLabels();

        [[nodiscard]] const std::shared_ptr<core::db::TaskTable>& getItems() const;

        [[nodiscard]] bool isExistNextPage() const;

        [[nodiscard]] bool isExistPrevPage() const;

        [[nodiscard]] int getCurrentPage() const;

        std::string getParentName();

        long long getParentId() const;

        [[nodiscard]] long long getTaskCount() const;

        void parentHistoryBack();


        [[nodiscard]] std::string formattedCurrentPage() const;

        void setStatusFilter(int i);

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
        std::function<void()> _task_on_selected;
    };

    class TaskListViewBase final : public ftxui::ComponentBase {
    public:
        explicit TaskListViewBase(const std::function<void(const std::string& msg_)>& on_error_);

        ftxui::Element OnRender() override;

        friend class TaskDetailBase;

    private:
        ftxui::Component HistoryBackButton();

        ftxui::Component NewTaskButton();

        ftxui::Component StatusFilterToggleMenu();

        ftxui::Component TaskListMenu();

        ftxui::Component PrevButton();

        ftxui::Component NextButton();

        std::shared_ptr<TaskDetailBase> TaskDetail();

        TaskListViewData _data;

        ftxui::Component _history_back_button;
        ftxui::Component _new_task_button;
        ftxui::Component _status_filter_toggle;
        ftxui::Component _pagination_button;
        ftxui::Component _prev_button;
        ftxui::Component _next_button;

        ftxui::Component _task_list_menu;
        std::shared_ptr<TaskDetailBase> _task_detail;
        ftxui::Component _main_component;

        ftxui::Box _task_list_box;
    };

    class ActiveTask final : public ftxui::ComponentBase {
    public:
        explicit ActiveTask(const std::function<void()>& on_jump_button_click_);

        ftxui::Element OnRender() override;

        void activate(long long task_id_);

        void deactivate();

        [[nodiscard]] const std::string& getActiveTaskName() const;

        const std::string& getTimerText();

        std::chrono::seconds getSeconds() const;

        [[nodiscard]] bool isActivated() const;

        [[nodiscard]] long long getActiveTaskId() const;

    private:
        void _loadActiveTask();

        void _activate();

        static void _onTimerUpdated();

        bool _is_activated{false};
        std::string _active_task_name{};
        long long _active_task_id{-1};
        DurationTimer _active_timer{0};
        std::function<void()> _on_jump_button_click;

        ftxui::Component _jump_button;
    };

    /**
     * @brief ここにクラスの説明
     * @details ここにクラスの詳細な説明
     * @since
     */
    class TaskDetailBase final : public ftxui::ComponentBase {
    public:
        explicit TaskDetailBase(TaskListViewBase* tasklist_view_base_);

        ftxui::Element OnRender() override;

        void selectedTaskChanged();

        void updateTotalWorktime();

    private:
        void _deleteTask();

        void _updateTask();

        void _jumpButtonOnClick() const;

        [[nodiscard]] bool _isNotCurrentTaskActivated() const;

        [[nodiscard]] bool _isCurrentTaskActivated() const;

        TaskListViewBase* _tasklist_view_base;
        std::shared_ptr<ActiveTask> _active_task;
        ftxui::Component _task_name_input;
        ftxui::Component _task_status_toggle;
        ftxui::Component _task_detail_input;
        ftxui::Component _worktime_summary;
        ftxui::Component _activate_task_button;
        ftxui::Component _deactivate_task_button;
        ftxui::Component _update_button;
        ftxui::Component _delete_button;

        std::string _task_name;
        int _selected_status{1};
        std::string _task_detail;
        std::chrono::seconds _total_worktime{0};
        static const std::vector<std::string> TASK_STATUS;
    };

    ftxui::Component TaskListView(const std::function<void(const std::string& msg_)>& on_error_);
} // components

#endif //TASKLISTVIEW_H
