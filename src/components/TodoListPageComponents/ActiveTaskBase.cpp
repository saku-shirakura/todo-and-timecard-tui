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

#include "TodoListPageComponents.h"

#include <ftxui/component/component.hpp>
#include "../../core/TodoAndTimeCardApp.h"
#include "../../utilities/Utilities.h"

namespace components {
    ActiveTaskBase::ActiveTaskBase(const std::function<void()>& on_jump_button_click_): _on_jump_button_click(
        on_jump_button_click_
            ? on_jump_button_click_
            : [] {
            })
    {
        _jump_button = ftxui::Button("jump", _on_jump_button_click, ftxui::ButtonOption::Ascii());
        Add(_jump_button);
        _loadActiveTask();
    }

    ftxui::Element ActiveTaskBase::OnRender()
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

    void ActiveTaskBase::activate(const long long task_id_)
    {
        core::db::WorktimeTable::activateTask(task_id_);
        _activate();
    }

    void ActiveTaskBase::deactivate()
    {
        core::db::WorktimeTable::deactivateAllTasks();
        _active_task_name = "";
        _active_task_id = -1;
        _active_timer.stop();
        _active_timer.setUpdateCallback(nullptr);
        _active_timer.setStartEpoch(0);
        _is_activated = false;
    }

    const std::string& ActiveTaskBase::getActiveTaskName() const { return _active_task_name; }

    const std::string& ActiveTaskBase::getTimerText() { return _active_timer.getText(); }

    std::chrono::seconds ActiveTaskBase::getSeconds() const
    {
        using namespace std::chrono_literals;
        return _active_timer.isActive() ? _active_timer.getSeconds() : 0s;
    }

    bool ActiveTaskBase::isActivated() const { return _is_activated; }

    long long ActiveTaskBase::getActiveTaskId() const { return _active_task_id; }

    void ActiveTaskBase::_loadActiveTask() { _activate(); }

    void ActiveTaskBase::_activate()
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

    void ActiveTaskBase::_onTimerUpdated() { core::TodoAndTimeCardApp::updateScreen(); }
}
