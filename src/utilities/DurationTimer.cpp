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

#include "Utilities.h"


DurationTimer::DurationTimer(const long long start_time_epoch_): DurationTimer(start_time_epoch_, nullptr)
{
}

DurationTimer::DurationTimer(const std::chrono::seconds start_time_epoch_): DurationTimer(start_time_epoch_, nullptr)
{
}

DurationTimer::DurationTimer(const long long start_time_epoch_, const std::function<void()>& on_update_):
    _start_time_epoch(start_time_epoch_), _on_update(on_update_
                                                         ? on_update_
                                                         : [] {
                                                         }) { _thread = std::thread([&] { _threadProcess(); }); }

DurationTimer::DurationTimer(const std::chrono::seconds start_time_epoch_,
                             const std::function<void()>& on_update_): DurationTimer(
    start_time_epoch_.count(), on_update_)
{
}

DurationTimer::~DurationTimer()
{
    _loop = false;
    if (_thread.joinable()) { _thread.detach(); }
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

const std::string& DurationTimer::getText()
{
    std::lock_guard lock(_update_text_mtx);
    return _duration_text;
}

void DurationTimer::setUpdateCallback(const std::function<void()>& on_update_)
{
    std::lock_guard lock(_on_update_mtx);
    _on_update = on_update_
                     ? on_update_
                     : [] {
                     };
}

std::chrono::seconds DurationTimer::getSeconds() const { return _duration_seconds; }

void DurationTimer::stop() { _active = false; }

void DurationTimer::start()
{
    if (_active) return;
    _active = true;
    _active_condition.notify_one();
}

bool DurationTimer::isActive() const { return _active; }

void DurationTimer::_updateCallback() noexcept
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
    const auto now_time_point = std::chrono::system_clock::now();
    const auto now_seconds = std::chrono::duration_cast<std::chrono::seconds>(now_time_point.time_since_epoch());
    _duration_seconds = now_seconds - _start_time_epoch;
    _duration_text = util::timeTextFromSeconds(_duration_seconds, true);
}

void DurationTimer::_threadProcess()
{
    using namespace std::chrono_literals;
    while (_loop) {
        if (!_active) {
            std::unique_lock lock(_stop_timer_mtx);
            _active_condition.wait(lock, [&] { return _active; });
        }
        _updateText();
        _updateCallback();
        std::this_thread::sleep_for(500ms);
    }
}
