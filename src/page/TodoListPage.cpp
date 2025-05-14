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

#include "../components/TaskListView.h"
#include "../core/DBManager.h"

namespace pages {
    TodoListPage::TodoListPage()
    {
        _page_container = ftxui::Container::Vertical({});
        _task_list_view = components::TaskListView([&](const std::string& msg) {
            _console_data->printConsole(msg);
        });

        _console = components::Console(_console_data);

        _page_container->Add(_task_list_view);
        _page_container->Add(_console);
    }

    ftxui::Component TodoListPage::getComponent() const
    {
        return Renderer(_page_container, [&] {
            return vbox(
                _task_list_view->Render(),
                ftxui::separator(),
                _console->Render()
            ) | size(ftxui::HEIGHT, ftxui::EQUAL, 30) | size(ftxui::WIDTH, ftxui::EQUAL, 120);
        });
    }
} // pages
