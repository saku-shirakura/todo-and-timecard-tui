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

#include "../../utilities/Utilities.h"

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
        setStatusFilter(0);
        resetPage();
    }

    void TaskListViewData::selectTask(const long long task_id_)
    {
        setStatusFilter(0);
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

    void TaskListViewData::parentHistoryBack() { selectTask(_parent_id); }

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
}
