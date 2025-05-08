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

#include "custom_menu_entry.h"

/**
 * 
 * @param state_
 * @param status_
 * @param disabled_
 * @return
 * @note このメソッドは、[ftxui::DefaultOptionTransform()](https://github.com/ArthurSonzogni/FTXUI/blob/v6.1.9/src/ftxui/component/menu.cpp)を改造して作成されています。
 * @note 改造元のオリジナルコードは[MIT LICENSE](https://github.com/ArthurSonzogni/FTXUI/tree/v6.1.9/LICENSE)のもと提供されています。
 * @note 改造部分については本ソフトウェアのLICENSEに従います。変更箇所の詳細は[元となったコード](https://github.com/ArthurSonzogni/FTXUI/blob/v6.1.9/src/ftxui/component/menu.cpp)と比較してください。
 */
ftxui::Element components::customize::TodoListMenuEntryOptionTransform(const ftxui::EntryState& state_, const core::db::Status status_, const bool disabled_)
{
    /*
     * This method was created by modifying [ftxui::DefaultOptionTransform()](https://github.com/ArthurSonzogni/FTXUI/blob/v6.1.9/src/ftxui/component/menu.cpp).
     * The original code is provided under [MIT LICENSE](https://github.com/ArthurSonzogni/FTXUI/tree/v6.1.9/LICENSE).
     * The modified parts are subject to the license of this software. For details, see [original code](https://github.com/ArthurSonzogni/FTXUI/blob/v6.1.9/src/ftxui/component/menu.cpp).
     */
    using namespace ftxui;
    // begin to modify
    if (disabled_) {
        return text("");
    }
    // end to modify
    std::string label = (state_.active ? "> " : "  ") + state_.label;  // NOLINT
    Element e = text(std::move(label));
    // begin to modify
    switch (status_) {
    case core::db::Status::Progress:
        e |= color(Color::BlueLight);
        break;
    case core::db::Status::Incomplete:
        e |= color(Color::RedLight);
        break;
    case core::db::Status::Complete:
        e |= color(Color::GreenLight);
        break;
    case core::db::Status::Not_Planned:
        e |= color(Color::GrayDark);
        break;
    }
    // end to modify
    if (state_.focused) {
        e = e | inverted;
    }
    if (state_.active) {
        e = e | bold;
    }
    return e;
}
