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

namespace pages {
    void TodoListPage::show()
    {
        ftxui::Component task_filter_toggle = ftxui::Toggle(&TASK_FILTER_MODE, &_current_task_filter_selected);
        const ftxui::Component task_filter_container = ftxui::Container::Vertical({task_filter_toggle});

        this->_renderer = Renderer(task_filter_container, [&] {
            return hbox(
                vbox(
                    task_filter_toggle->Render(),
                    ftxui::separator()
                ) | ftxui::border
            );
        });

        _render();
    }

    const std::vector<std::string> TodoListPage::TASK_FILTER_MODE{
        "All", "In progress", "Incompleted", "Completed", "Not planned"
    };
} // pages
