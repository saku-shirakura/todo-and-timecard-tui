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
 * @file DurationTimer.h
 * @date 25/05/11
 * @brief ファイルの説明
 * @details ファイルの詳細
 * @author saku shirakura (saku@sakushira.com)
 */


#ifndef DURATIONTIMER_H
#define DURATIONTIMER_H
#include <chrono>
#include <condition_variable>
#include <functional>
#include <string>
#include <thread>
#include <mutex>


/**
 * @brief ここにクラスの説明
 * @details ここにクラスの詳細な説明
 * @since
 */
class DurationTimer {
public:
    explicit DurationTimer(long long start_time_epoch_);
    explicit DurationTimer(std::chrono::seconds start_time_epoch_);
    explicit DurationTimer(long long start_time_epoch_, const std::function<void()>& on_update_);
    explicit DurationTimer(std::chrono::seconds start_time_epoch_, const std::function<void()>& on_update_);

    ~DurationTimer();

    void setStartEpoch(long long start_time_epoch_);

    void setStartEpoch(std::chrono::seconds start_time_epoch_);

    [[nodiscard]] const std::string& getText();

    void setUpdateCallback(const std::function<void()>& on_update_);

    std::chrono::seconds getSeconds() const;

    void stop();

    void start();

    bool isActive() const;

private:
    void _updateCallback() noexcept;

    void _updateText();

    void _threadProcess();

    std::thread _thread;
    std::chrono::seconds _start_time_epoch;
    std::chrono::seconds _duration_seconds;
    std::string _duration_text{};
    std::function<void()> _on_update{};

    std::mutex _update_text_mtx;
    std::mutex _on_update_mtx;
    std::mutex _stop_timer_mtx;

    std::condition_variable _active_condition;
    bool _active{false};

    bool _loop{true};
};


#endif //DURATIONTIMER_H
