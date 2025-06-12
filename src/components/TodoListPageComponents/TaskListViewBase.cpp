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
#include <ftxui/component/event.hpp>
#include "../custom_menu_entry.h"

namespace components {
    TaskListViewBase::TaskListViewBase(
        const std::function<void(const std::string& msg_)>& on_error_
    )
        :
        _data(on_error_,
              [&] { _task_detail->selectedTaskChanged(); })
    {
        // トップメニュー
        _history_back_button = HistoryBackButton();
        _new_task_button = NewTaskButton();

        const auto current_task_menu = ftxui::Container::Horizontal({});
        current_task_menu->Add(_history_back_button);
        current_task_menu->Add(_new_task_button);

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
        _main_component->Add(current_task_menu);
        _main_component->Add(task_area);
        _main_component->Add(_pagination_button);
        Add(_main_component);
    }

    ftxui::Element TaskListViewBase::OnRender()
    {
        return
            hbox(
                vbox(
                    hbox(
                        _history_back_button->Render(),
                        ftxui::text(" "),
                        ftxui::text(_data.getParentName()),
                        ftxui::filler(),
                        _new_task_button->Render()
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

    ftxui::Component TaskListViewBase::HistoryBackButton()
    {
        ftxui::ButtonOption button_option = ftxui::ButtonOption::Ascii();

        button_option.label = "↑";
        button_option.on_click = [&] { _data.parentHistoryBack(); };

        return ftxui::Button(button_option);
    }

    ftxui::Component TaskListViewBase::NewTaskButton()
    {
        ftxui::ButtonOption button_option = ftxui::ButtonOption::Ascii();

        button_option.label = "+";
        button_option.on_click = [&] {
            const auto parent_id = _data.getParentId();
            const auto insert_err = core::db::TaskTable::newTask(parent_id);
            if (insert_err != 0) return;
            auto [select_err, task] = core::db::TaskTable::fetchLastTask(_data.getParentId());
            if (select_err != 0) return;
            _data.selectTask(task.id);
        };

        return ftxui::Button(button_option);
    }

    ftxui::Component TaskListViewBase::StatusFilterToggleMenu()
    {
        auto toggle = ftxui::MenuOption::Toggle();
        toggle.on_change = [&] {
            _data.resetPage();
            _task_detail->selectedTaskChanged();
        };
        return ftxui::Menu(&TaskListViewData::TASK_FILTER_MODE, _data.getSelectedStatusFilter(), toggle);
    }

    ftxui::Component TaskListViewBase::TaskListMenu()
    {
        ftxui::MenuOption task_list_option{};
        task_list_option.entries = _data.getTaskLabels().get();
        task_list_option.selected = _data.getSelectedTaskPtr();
        task_list_option.focused_entry = _data.getFocusedTaskPtr();
        task_list_option.on_change = [&] {
            _data.taskListOnChange();
            _task_detail->selectedTaskChanged();
        };
        task_list_option.on_enter = [&] {
            _data.taskListOnEnter();
            _task_detail->selectedTaskChanged();
        };
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
                        _task_detail->selectedTaskChanged();
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

    ftxui::Component TaskListViewBase::PrevButton()
    {
        ftxui::ButtonOption prev_button_option = ftxui::ButtonOption::Ascii();
        prev_button_option.label = "←";
        prev_button_option.on_click = [&] { _data.prevPage(); };
        return ftxui::Button(prev_button_option);
    }

    ftxui::Component TaskListViewBase::NextButton()
    {
        ftxui::ButtonOption next_button_option = ftxui::ButtonOption::Ascii();
        next_button_option.label = "→";
        next_button_option.on_click = [&] { _data.nextPage(); };
        return ftxui::Button(next_button_option);
    }

    std::shared_ptr<TaskDetailBase> TaskListViewBase::TaskDetail() { return ftxui::Make<TaskDetailBase>(this); }
}
