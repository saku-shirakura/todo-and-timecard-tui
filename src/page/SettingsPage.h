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

/**
 * @file SettingsPage.h
 * @date 25/06/14
 * @brief ファイルの説明
 * @details ファイルの詳細
 * @author saku shirakura (saku@sakushira.com)
 */


#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H
#include <ftxui/component/component.hpp>
#include "../components/TodoListPageComponents/TodoListPageComponents.h"

namespace pages {
    /**
     * @brief ここにクラスの説明
     * @details ここにクラスの詳細な説明
     * @since
     */
    class SettingsPage final {
    public:
        SettingsPage();

        ftxui::Component getComponent() const;

    private:
        class SettingEntryImpl final : public ftxui::ComponentBase {
        public:
            static std::shared_ptr<SettingEntryImpl> create(std::string setting_key_,
                                                            std::vector<std::string> menu_entry_);

            ftxui::Element OnRender() override;

            void setOnChange(const std::function<void(std::string prev_value_, std::string value_)>& handler_);

            SettingEntryImpl(std::string setting_key_, std::vector<std::string> menu_entry_);

        private:
            ftxui::Component _component;
            std::string _setting_key;
            std::string _setting_value{};
            int _selection_selected{};
            std::vector<std::string> _selections;
            std::function<void(std::string prev_value_, std::string value_)> _on_change;
        };

        ftxui::Component _container;
        std::vector<std::shared_ptr<SettingEntryImpl>> _entries{};
    };
} // pages

#endif //SETTINGSPAGE_H
