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

#include "SettingsPage.h"

#include <ftxui/dom/elements.hpp>

#include "../core/Logger.h"

namespace pages {
    SettingsPage::SettingsPage()
    {
        _entries.push_back(SettingEntryImpl::create("Timezone", {
                                                        "-1200",
                                                        "-1100",
                                                        "-1000",
                                                        "-0900",
                                                        "-0800",
                                                        "-0700",
                                                        "-0600",
                                                        "-0500",
                                                        "-0400",
                                                        "-0300",
                                                        "-0200",
                                                        "-0100",
                                                        "+0000",
                                                        "+0100",
                                                        "+0200",
                                                        "+0300",
                                                        "+0400",
                                                        "+0500",
                                                        "+0600",
                                                        "+0700",
                                                        "+0800",
                                                        "+0900",
                                                        "+1000",
                                                        "+1100",
                                                        "+1200",
                                                        "+1300",
                                                        "+1400",
                                                    }));
        _entries.push_back(SettingEntryImpl::create("LogLevel", {
                                                        "debug",
                                                        "info",
                                                        "warning",
                                                        "error",
                                                        "critical"
                                                    }));
        _entries.back()->setOnChange([&](std::string p, std::string v) { Logger::loadFromSettings(); });

        _container = ftxui::Container::Vertical({});
        for (size_t i = 0; i < _entries.size(); i++) { _container->Add(_entries.at(i)); }
    }

    ftxui::Component SettingsPage::getComponent() const
    {
        return Renderer(_container, [&] {
            return _container->Render() |
                ftxui::yflex_grow |
                ftxui::frame |
                ftxui::vscroll_indicator;
        });
    }

    std::shared_ptr<SettingsPage::SettingEntryImpl> SettingsPage::SettingEntryImpl::create(
        std::string setting_key_, std::vector<std::string> menu_entry_)
    {
        return std::make_shared<SettingEntryImpl>(setting_key_, menu_entry_);
    }

    ftxui::Element SettingsPage::SettingEntryImpl::OnRender()
    {
        return ftxui::hbox(
            ftxui::text(_setting_key) | ftxui::vcenter,
            ftxui::filler(),
            _component->Render()
        );
    }

    void SettingsPage::SettingEntryImpl::setOnChange(
        const std::function<void(std::string prev_value_, std::string value_)>& handler_) { _on_change = handler_; }

    SettingsPage::SettingEntryImpl::SettingEntryImpl(std::string setting_key_, std::vector<std::string> menu_entry_):
        _setting_key(std::move(setting_key_)),
        _selections(std::move(menu_entry_))
    {
        ftxui::DropdownOption option{};
        if (!_selections.empty()) {
            core::db::SettingTable tbl{};
            tbl.selectRecords("setting_key = ?", {
                                  {core::db::ColType::T_TEXT, _setting_key}
                              });

            if (!tbl.getKeys().empty())
                if (const auto i = std::ranges::find(_selections, tbl.getTable().at(tbl.getKeys().front()).value);
                    i == _selections.end()) { _selection_selected = 0; }
                else { _selection_selected = std::distance(_selections.begin(), i); }
        }
        option.radiobox.on_change = [&] {
            if (_selection_selected >= 0 && _selection_selected < _selections.size()) {
                const std::string prev = _setting_value;
                _setting_value = _selections.at(_selection_selected);
                core::db::NoMappingTable tbl;
                tbl.usePlaceholderUniSql("UPDATE settings SET value = ? WHERE setting_key = ?;", {
                                             {
                                                 core::db::ColType::T_TEXT,
                                                 _setting_value
                                             },
                                             {
                                                 core::db::ColType::T_TEXT,
                                                 _setting_key
                                             }
                                         });
                if (_on_change) _on_change(prev, _setting_value);
            }
        };
        option.radiobox.entries = &_selections;
        option.radiobox.selected = &_selection_selected;

        _component = ftxui::Dropdown(option);
        Add(_component);
    }
} // pages
