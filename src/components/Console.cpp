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

#include "Console.h"

namespace components {
    class ConsoleBase final : public ftxui::ComponentBase {
    public:
        explicit ConsoleBase(std::shared_ptr<ConsoleData> data_): _data(std::move(data_))
        {
            auto console = ftxui::Scroller(
                ftxui::Renderer([&] { return ftxui::paragraph(_data->getText()); })
            );
            Add(std::move(console));
        }

        ftxui::Element OnRender() override
        {
            const ftxui::Element console_view = Render();
            return window(
                ftxui::text("console"),
                console_view
            );;
        }

    private:
        std::shared_ptr<ConsoleData> _data;
    };

    void ConsoleData::printConsole(const std::string& str_)
    {
        _console_history.push_back(str_);
        if (_console_history.size() > 20) { _console_history.erase(_console_history.begin()); }
        _console_text = "";
        for (size_t i = _console_history.size(); i > 0; i--) {
            _console_text += _console_history.at(i - 1);
            if (i != 1)
                _console_text += "\n";
        }
    }

    std::string ConsoleData::getText() { return _console_text; }

    ftxui::Component Console(std::shared_ptr<ConsoleData> console_data_)
    {
        return ftxui::Make<ConsoleBase>(std::move(console_data_));
    }
} // components
