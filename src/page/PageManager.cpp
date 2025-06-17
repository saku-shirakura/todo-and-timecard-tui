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

#include "PageManager.h"

namespace pages {
    PageManager::PageManager()
    {
        // Register Page Titles
        _tab_names.emplace_back("TodoList");
        _tab_names.emplace_back("Worktime");
        _tab_names.emplace_back("Settings");

        ftxui::MenuOption switcher_option = ftxui::MenuOption::Toggle();
        switcher_option.on_change = [&] {
            if (_tab_names.at(_selected_page) == "TodoList") _todo_list_page.onShowing();
            else if (_tab_names.at(_selected_page) == "Worktime") _worktime_summary_page.onShowing();
            else if (_tab_names.at(_selected_page) == "Settings") _settings_page.onShowing();
        };
        _tab_switcher = ftxui::Menu(&_tab_names, &_selected_page, switcher_option);

        // Register Pages
        _page_container->Add(_todo_list_page.getComponent());
        _page_container->Add(_worktime_summary_page.getComponent());
        _page_container->Add(_settings_page.getComponent());

        // Assemble main content.
        _container->Add(_page_container);
        _container->Add(_tab_switcher);
    }

    ftxui::Component PageManager::getComponent() const
    {
        return Renderer(_container, [&] {
            return vbox(
                _page_container->Render() | ftxui::border | decorator::pageDecorator,
                ftxui::separator(),
                _tab_switcher->Render()
            );
        });
    }
}
