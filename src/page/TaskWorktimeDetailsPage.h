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
 * @file TaskWorktimeDetails.h
 * @date 25/06/13
 * @brief ファイルの説明
 * @details ファイルの詳細
 * @author saku shirakura (saku@sakushira.com)
 */


#ifndef TASKWORKTIMEDETAILS_H
#define TASKWORKTIMEDETAILS_H
#include <ftxui/component/component.hpp>

namespace pages {
    /**
     * @brief ここにクラスの説明
     * @details ここにクラスの詳細な説明
     * @since
     */
    class TaskWorktimeDetailsPage final {
    public:
        TaskWorktimeDetailsPage();

        [[nodiscard]] ftxui::Component getComponent() const;

        void setTaskId(long long task_id_) const;

        long long getTaskId() const;

        std::shared_ptr<long long> getPtrTaskId();

    private:
        std::shared_ptr<long long> _task_id{new long long()};
    };
} // components

#endif //TASKWORKTIMEDETAILS_H
