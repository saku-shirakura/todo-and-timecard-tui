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

#include "GanttChartLine.h"

#include <cmath>

#include "../utilities/Utilities.h"

namespace {
}

namespace elements {
    ftxui::Element GanttChartLine(const std::string& label_,
                                  const long long base_seconds_,
                                  std::vector<std::pair<long long, long long>> timelines_, const bool focused_)
    {
        constexpr int right_margin = 16;
        constexpr int width = 216;
        constexpr int height = 4;
        constexpr int label_area = width / 5;
        auto canvas = ftxui::Canvas(width + right_margin, height);
        for (const auto [start_time, end_time] : timelines_) {
            constexpr long long minimum_seconds = 0;
            constexpr long long maximum_seconds = 86400;
            if (start_time > end_time) continue;
            const long long start_seconds = util::fitInt(start_time - base_seconds_, maximum_seconds, minimum_seconds);
            const long long end_seconds = util::fitInt(end_time - base_seconds_, maximum_seconds, minimum_seconds);
            const double start_sec_percentage = start_seconds / static_cast<double>(maximum_seconds);
            const double end_sec_percentage = end_seconds / static_cast<double>(maximum_seconds);
            const double x1 = start_sec_percentage * static_cast<double>(width - label_area) + label_area;
            const double x2 = end_sec_percentage * static_cast<double>(width - label_area) + label_area;
            std::string label = util::ellipsisString(label_, label_area * 0.45 - 2);
            label = (focused_ ? "* " : "  ") + label;
            canvas.DrawText(0, 0, label);
            canvas.DrawBlockLine(round(x1), 0, round(x2), 0);
        }
        return ftxui::canvas(std::move(canvas));
    }

    ftxui::Element GanttChartTimeMeasure()
    {
        constexpr int right_margin = 16;
        constexpr int width = 216;
        constexpr int height = 4;
        constexpr int label_area = width / 5;
        auto canvas = ftxui::Canvas(width + right_margin, height);
        for (int i = 0; i <= 24; i++) {
            constexpr long long maximum_seconds = 86400;
            const long long sec = i * 3600;
            const double hour_percentage = sec / static_cast<double>(maximum_seconds);
            const double hour_pos = hour_percentage * static_cast<double>(width - label_area) + label_area;
            if (i % 3 == 0) {
                ftxui::Color::Palette256 color;
                if (i % 2 == 0)
                    color = ftxui::Color::Yellow1;
                else
                    color = ftxui::Color::Orange1;
                canvas.DrawPointLine(round(hour_pos), 0, round(hour_pos), 0, color);
            }
            else { canvas.DrawPointLine(round(hour_pos), 0, round(hour_pos), 0); }
            canvas.DrawText(label_area / 2 - 4, 0, "task");
        }
        return ftxui::canvas(std::move(canvas));
    }
} // elements
