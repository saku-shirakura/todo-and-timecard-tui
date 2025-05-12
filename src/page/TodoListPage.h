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
#include <stack>
#include <string>
#include <vector>

#include "../../cmake-build-release/_deps/ftxui-src/include/ftxui/component/screen_interactive.hpp"
#include "../components/Console.h"
#include "../core/DBManager.h"

namespace pages {
    class TodoListPage final {
    public:
        TodoListPage();

        ftxui::Component getComponent();

    private:
        ftxui::Component _page_container;
        std::shared_ptr<components::ConsoleData> _console_data{new components::ConsoleData};
    };
} // pages

#endif //TODOLISTPAGE_H
