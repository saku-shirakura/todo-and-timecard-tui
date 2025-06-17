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
 * @file GanttChartLine.h
 * @date 25/06/16
 * @brief ファイルの説明
 * @details ファイルの詳細
 * @author saku shirakura (saku@sakushira.com)
 */


#ifndef GANTTCHARTLINE_H
#define GANTTCHARTLINE_H
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/canvas.hpp>

namespace elements {
    /**
     * @brief １日分のガントチャートを基準秒と開始日時・終了日時のリストから表示します。
     * @details
     * @param label_
     * @param base_seconds_ 基準秒。基本的には、対象の日付における00:00:00を指定します。
     * @param timelines_ 開始日時と終了日時のぺアによるリスト。日時からは、基準秒が減じられます。
     *  日時 - 基準秒の値が範囲を超える場合は、最も近い基準値に補正されます。
     *  開始 > 終了 の場合は無視されます。
     * @param focused_
     * @since
     */
    ftxui::Element GanttChartLine(
        const std::string& label_,
        long long base_seconds_,
        std::vector<std::pair<long long, long long>> timelines_, bool focused_ = false
    );

    ftxui::Element GanttChartTimeMeasure();
} // elements

#endif //GANTTCHARTLINE_H
