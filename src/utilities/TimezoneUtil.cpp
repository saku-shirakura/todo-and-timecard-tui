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

#include "TimezoneUtil.h"

#include <string>

#include "Utilities.h"
#include "../core/DBManager.h"

long long util::tz::fetchDifferenceSeconds()
{
    core::db::SettingTable tbl;
    tbl.selectRecords("setting_key = 'timezone'", {});
    if (tbl.getKeys().empty()) return 0;
    const auto tz_config = tbl.getTable().at(tbl.getKeys().front()).value;
    const int raw_difference = std::stoi(tz_config);
    const int abs_difference = abs(raw_difference);
    const int difference_min = fitInt(abs_difference % 100, 59, 0);
    const int difference_hour = fitInt(abs_difference / 100 % 100, 24, 0);
    const int difference = (difference_hour * 3600 + difference_min * 60) * (raw_difference < 0 ? -1 : 1);
    return difference;
}

long long util::tz::addTimezoneValue(const long long _unix_epoch) { return _unix_epoch + fetchDifferenceSeconds(); }
