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

#include "DurationTimer.h"

#include <iostream>

DurationTimer::DurationTimer(const long long start_time_epoch_): DurationTimer(start_time_epoch_, nullptr)
{
    _thread = std::thread([&] { _threadProcess(); });
}

DurationTimer::DurationTimer(const std::chrono::seconds start_time_epoch_): DurationTimer(start_time_epoch_, nullptr)
{
}

DurationTimer::DurationTimer(const long long start_time_epoch_, const std::function<void()>& on_update_):
    _start_time_epoch(start_time_epoch_), _on_update(on_update_
                                                         ? on_update_
                                                         : [] {
                                                         })
{
}

DurationTimer::DurationTimer(const std::chrono::seconds start_time_epoch_,
                             const std::function<void()>& on_update_): DurationTimer(
    start_time_epoch_.count(), on_update_)
{
}

DurationTimer::~DurationTimer()
{
    _loop = false;
    if (_thread.joinable()) { _thread.join(); }
}

void DurationTimer::setStartEpoch(const long long start_time_epoch_)
{
    setStartEpoch(std::chrono::seconds(start_time_epoch_));
}

void DurationTimer::setStartEpoch(const std::chrono::seconds start_time_epoch_)
{
    std::lock_guard lock(_update_text_mtx);
    _start_time_epoch = start_time_epoch_;
}

const std::string& DurationTimer::getText() const
{
    std::lock_guard lock(_update_text_mtx);
    return _duration_text;
}

void DurationTimer::setUpdateCallback(const std::function<void()>& on_update_)
{
    std::lock_guard lock(_on_update_mtx);
    _on_update = on_update_;
}

void DurationTimer::_updateCallback() const noexcept
{
    try {
        std::lock_guard lock(_on_update_mtx);
        _on_update();
    }
    catch (const std::exception& _) {
    }
}

void DurationTimer::_updateText()
{
    std::lock_guard lock(_update_text_mtx);
    using namespace std::chrono_literals;
    const auto now_time_point = std::chrono::utc_clock::now();
    const auto now_seconds = std::chrono::duration_cast<std::chrono::seconds>(now_time_point.time_since_epoch());
    const auto raw_seconds = now_seconds - _start_time_epoch;
    const auto seconds = raw_seconds.count() % 60;
    const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(raw_seconds).count() % 60;
    const auto hours = std::chrono::duration_cast<std::chrono::hours>(raw_seconds).count() % 24;
    const auto days = std::chrono::duration_cast<std::chrono::days>(raw_seconds).count() % 30;
    const auto months = std::chrono::duration_cast<std::chrono::months>(raw_seconds).count() % 12;
    const auto years = std::chrono::duration_cast<std::chrono::years>(raw_seconds).count();
    if (years > 0) { _duration_text = std::format("{:02}Y{:02}M", years, months); }
    else if (months > 0) { _duration_text = std::format("{:02}M{:02}D", months, days); }
    else if (days > 0) { _duration_text = std::format("{:02}D{:02}h", days, hours); }
    else if (hours > 0) { _duration_text = std::format("{:02}h{:02}m", hours, minutes); }
    else { _duration_text = std::format("{:02}m{:02}s", minutes, seconds); }
}

void DurationTimer::_threadProcess()
{
    using namespace std::chrono_literals;
    while (_loop) {
        _updateText();
        _updateCallback();
        std::this_thread::sleep_for(500ms);
    }
}
