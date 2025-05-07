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
* @file TodoListPage.h
* @date 25/05/01
* @brief 簡単な説明(不要なら削除すること。)
* @details 詳細(不要なら削除すること。)
* @author saku shirakura (saku@sakushira.com)
* @since 
*/

#ifndef TODOLISTPAGE_H
#define TODOLISTPAGE_H
#include <string>
#include <vector>

#include "PageBase.h"
#include "../core/DBManager.h"

namespace pages {
    class TodoListPage final : PageBase {
    public:
        void show();

        void updateTaskList();

        void resetTaskList();

        void nextTaskList();

        void prevTaskList();

    private:
        void _updateTaskItemCount();

        bool _eventHandler(ftxui::Event event_);

        int _filterSelectedStatusId() const;

        int _current_task_filter_selected{0};

        void printConsole(const std::string& str_);

        long long _tasks_count = 0;
        int _task_page = 0;
        int _tasks_per_page = 20;

        long long _current_parent_id{0};

        std::string _console_text{"test1\ntest2\ntest3\ntest4\ntest5\ntest6\ntest7\n"};

        std::vector<std::string> _console_history{};

        std::vector<std::string> _task_labels{};
        core::db::TaskTable _task_items;
        int _selected_task = 0;

        static const std::vector<std::string> TASK_FILTER_MODE;
        static const std::unordered_map<std::string, core::db::Status> FILTER_LABEL_STATUS_MAP;
    };
} // pages

#endif //TODOLISTPAGE_H
