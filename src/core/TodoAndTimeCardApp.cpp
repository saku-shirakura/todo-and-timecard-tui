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

#include "TodoAndTimeCardApp.h"

#include "../page/PageManager.h"

namespace core {
    void TodoAndTimeCardApp::execute()
    {
        std::lock_guard lock(_screen_mutex);
        const pages::PageManager page{};
        _screen.Loop(page.getComponent() | ftxui::Modal(_error_dialog, &_show_error_dialog));
    }

    void TodoAndTimeCardApp::updateScreen() { _screen.PostEvent(ftxui::Event::Custom); }

    void TodoAndTimeCardApp::setError(const std::string& msg) { _error_dialog->setError(msg); }

    void TodoAndTimeCardApp::show() { _show_error_dialog = true; }

    void TodoAndTimeCardApp::close() { _show_error_dialog = false; }

    ftxui::ScreenInteractive TodoAndTimeCardApp::_screen{ftxui::ScreenInteractive::TerminalOutput()};
    std::mutex TodoAndTimeCardApp::_screen_mutex;
    std::shared_ptr<components::ErrorDialogBase> TodoAndTimeCardApp::_error_dialog{
        components::ErrorDialog([] { _show_error_dialog = false; })
    };
    bool TodoAndTimeCardApp::_show_error_dialog{false};
} // core
