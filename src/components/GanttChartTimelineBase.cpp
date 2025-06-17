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

#include "GanttChartTimelineBase.h"

#include <chrono>
#include "../elements/GanttChartLine.h"
#include "../utilities/TimezoneUtil.h"
#include "../page/SettingsPage.h"

using namespace std::chrono_literals;

namespace components {
    GanttChartTimelineBase::GanttChartTimelineBase()
    {
        const auto now = std::chrono::system_clock::now() + std::chrono::seconds(util::tz::fetchDifferenceSeconds());
        const auto seconds = std::chrono::floor<std::chrono::seconds>(now);
        const auto days = std::chrono::floor<std::chrono::days>(seconds);
        _date = std::chrono::year_month_day(days);
        updateDateStr();
        update();

        _date_control = ftxui::Container::Horizontal({});

        _prev_day_button = ftxui::Button("←", [&] {
            decreaseDay();
            updateDateStr();
            update();
        }, ftxui::ButtonOption::Ascii());

        _next_day_button = ftxui::Button("→", [&] {
            increaseDay();
            updateDateStr();
            update();
        }, ftxui::ButtonOption::Ascii());

        _date_control->Add(_prev_day_button);
        _date_control->Add(_next_day_button);

        pages::SettingsPage::registerEventOnChange("timezone", [&] { update(); });

        ftxui::MenuOption option = ftxui::MenuOption::Vertical();
        option.entries_option.transform = [&](const ftxui::EntryState& state) {
            const auto tbl = _task_tbl.getTable();
            const auto id = stoll(state.label);
            if (!tbl.contains(id) || !_worktime_data.contains(id))
                return elements::GanttChartLine(
                    state.label, 0, {}, state.focused);
            return elements::GanttChartLine(tbl.at(id).name,
                                            _date_sec.count(),
                                            _worktime_data.at(id),
                                            state.focused);
        };

        option.on_enter = [&] {
        };

        _gantt_chart = ftxui::Menu(&_task_keys, &_entered_task, option);

        _component = ftxui::Container::Vertical({});
        _component->Add(_date_control);
        _component->Add(_gantt_chart);
        Add(_component);
    }

    ftxui::Element GanttChartTimelineBase::OnRender()
    {
        return vbox(
            ftxui::hbox(
                _prev_day_button->Render(),
                ftxui::separator(),
                ftxui::filler(),
                ftxui::text(_date_str),
                ftxui::filler(),
                ftxui::separator(),
                _next_day_button->Render()
            ),
            ftxui::separator(),
            elements::GanttChartTimeMeasure(),
            _gantt_chart->Render() | ftxui::frame | ftxui::vscroll_indicator
        );
    }

    void GanttChartTimelineBase::update()
    {
        const std::chrono::sys_days d(_date);
        const std::chrono::sys_seconds sec(d);
        const auto difference = util::tz::fetchDifferenceSeconds();
        const auto starting_at = sec.time_since_epoch().count() - difference;
        const auto finishing_at = (sec + 86400s).time_since_epoch().count() - difference;
        _date_sec = sec.time_since_epoch();

        _task_tbl.selectRecords(
            "id IN (SELECT task_id FROM null_set_worktime WHERE (starting_time < ?1 AND finishing_time > ?2)"
            " OR starting_time BETWEEN ?1 AND ?2"
            " OR finishing_time BETWEEN ?1 AND ?2)", {
                {core::db::ColType::T_INTEGER, starting_at},
                {core::db::ColType::T_INTEGER, finishing_at}
            });

        // 対象のタスクを抽出
        _worktime_target_task_tbl.selectWorktimeExistTaskFromPeriod(starting_at, finishing_at);
        _task_keys.clear();
        _task_keys.reserve(_worktime_target_task_tbl.getKeys().size());
        const auto tmp_task_tbl = _worktime_target_task_tbl.getTable();
        for (const auto i : _worktime_target_task_tbl.getKeys()) {
            if (!tmp_task_tbl.contains(i)) continue;
            _task_keys.emplace_back(
                std::to_string(tmp_task_tbl.at(i).task_id)
            );
        }

        // 対象の作業時間を抽出
        _worktime_tbl.selectRecords("(starting_time < ?1 AND finishing_time > ?2)"
                                    " OR starting_time BETWEEN ?1 AND ?2"
                                    " OR finishing_time BETWEEN ?1 AND ?2",
                                    {
                                        {core::db::ColType::T_INTEGER, starting_at},
                                        {core::db::ColType::T_INTEGER, finishing_at}
                                    });
        _worktime_data.clear();
        const auto tmp_worktime_tbl = _worktime_tbl.getTable();
        for (const auto i : _worktime_tbl.getKeys()) {
            if (tmp_worktime_tbl.contains(i)) {
                _worktime_data.try_emplace(tmp_worktime_tbl.at(i).task_id,
                                           std::vector<std::pair<long long, long long>>{});
                _worktime_data.at(tmp_worktime_tbl.at(i).task_id).emplace_back(
                    tmp_worktime_tbl.at(i).starting_time + difference,
                    tmp_worktime_tbl.at(i).finishing_time + difference
                );
            }
        }
    }

    void GanttChartTimelineBase::updateDateStr() { _date_str = std::format("{:%F}", _date); }

    void GanttChartTimelineBase::increaseDay()
    {
        // 月末日の場合
        if (_date == std::chrono::year_month_day{_date.year() / _date.month() / std::chrono::last}) {
            // 日を1に変更し、月を1加算する。
            _date = std::chrono::year_month_day{_date.year() / _date.month() / 1} + std::chrono::months(1);
        }
        else {
            // 日に1を加える。
            _date = std::chrono::year_month_day{_date.year() / _date.month() / (_date.day() + std::chrono::days(1))};
        }
    }

    void GanttChartTimelineBase::decreaseDay()
    {
        // 月初日の場合
        if (_date == std::chrono::year_month_day{_date.year() / _date.month() / 1}) {
            // 月に1減じた日付を求める。
            const auto tmp = std::chrono::year_month_day{_date.year() / _date.month() / 1} - std::chrono::months(1);
            // 1を減じた月を基に、最終日を設定する。
            _date = std::chrono::year_month_day(tmp.year() / tmp.month() / std::chrono::last);
        }
        else {
            // 日付から1日を減ずる。
            _date = std::chrono::year_month_day{_date.year() / _date.month() / (_date.day() - std::chrono::days(1))};
        }
    }
} // components
