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
 * @file GanttChartTimelineBase.h
 * @date 25/06/16
 * @brief ファイルの説明
 * @details ファイルの詳細
 * @author saku shirakura (saku@sakushira.com)
 */


#ifndef GANNTCHARTTIMELINEBASE_H
#define GANNTCHARTTIMELINEBASE_H
#include <ftxui/component/component.hpp>

#include "../core/DBManager.h"

namespace components {
    /**
     * @brief ここにクラスの説明
     * @details ここにクラスの詳細な説明
     * @since
     */
    class GanttChartTimelineBase final : public ftxui::ComponentBase {
    public:
        GanttChartTimelineBase();

        ftxui::Element OnRender() override;

        void update();

        void updateDateStr();

        void increaseDay();

        void decreaseDay();

    private:
        std::chrono::year_month_day _date{};
        std::chrono::seconds _date_sec{};

        ftxui::Component _component;
        ftxui::Component _date_control;
        ftxui::Component _gantt_chart;
        ftxui::Component _next_day_button;
        ftxui::Component _prev_day_button;
        core::db::WorktimeTable _worktime_tbl;
        core::db::WorktimeTable _worktime_target_task_tbl;
        core::db::TaskTable _task_tbl;
        std::unordered_map<long long, std::vector<std::pair<long long, long long>>> _worktime_data{};
        std::string _date_str;

        std::vector<std::string> _task_keys;
        int _entered_task{};
    };
} // components

#endif //GANNTCHARTTIMELINEBASE_H
