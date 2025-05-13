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

#include "Utilities.h"

#include <format>


std::string util::getEnv(const std::string& variable_name_, const std::string& default_value_)
{
    const char* env = std::getenv(variable_name_.c_str());
    if (env == nullptr)
        return default_value_;
    std::string result = env;
    return result;
}

std::string util::getDataPath(const std::string& filename_)
{
    try {
        // Windowsなら、APPDATAをベースディレクトリとし、それ以外ではHOMEとする。
        std::filesystem::path path = getEnv(
#ifdef WIN32
            "APPDATA"
#else
        "HOME"
#endif
        );

        // アプリのベースディレクトリ
        path /= ".net.ln3.todo-and-timecard-tui";

        // ファイル名を加えたパス
        if (!filename_.empty()) { path /= std::filesystem::path{filename_}; }
        return {path.generic_string()};
    }
    catch (const std::exception&) { return ""; }
}

size_t util::countUtf8Character(const std::string& str_)
{
    size_t count = 0;
    for (const unsigned char i : str_) {
        const int size = utf8CharSize(i);
        if (size <= 0) continue;
        if (size > 1) { count += 2; }
        else { count++; }
    }
    return count;
}

int util::utf8CharSize(const unsigned char c)
{
    if ((c & 0x80) == 0x00) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    if ((c & 0xC0) == 0x80) return 0;
    return -1;
}

std::string util::utf8FitStrLength(const std::string& str_, const size_t length_)
{
    std::string result{};
    // 現在の文字数、残り文字数
    size_t current_character_count = 0;
    int byte_remaining = 0;

    if (countUtf8Character(str_) <= length_) return str_;

    // 文字列を処理する。
    for (const unsigned char i : str_) {
        // 残りバイトが存在しないなら
        if (byte_remaining <= 0) {
            const int char_size = utf8CharSize(i);
            // 文字数が既に既定に達している場合は終了する。
            if (current_character_count + char_size > length_) {
                break;
            }

            // 残りバイト数を更新する。
            byte_remaining = char_size;

            // 現在の文字数を更新する。
            if (byte_remaining == 1) {
                current_character_count++;
            } else if (byte_remaining >= 2){
                current_character_count += 2;
            }
        }
        // 残りバイトがあるなら、それを追記する。
        if (byte_remaining > 0) {
            result.push_back(i);
            byte_remaining--;
        }
    }
    return result;
}

std::string util::ellipsisString(const std::string& str_, const size_t max_length_, const std::string& ellipsis_)
{
    // 表示上の省略記号の文字数を求める。
    const size_t actual_ellipsis_length = countUtf8Character(ellipsis_);
    if (actual_ellipsis_length > max_length_) return ellipsis_;

    // 表示上の文字数が最大値を超えない場合は何もしない。
    if (countUtf8Character(str_) <= max_length_) return str_;

    // 省略記号分を除いた使用可能な文字数を求める。
    const size_t ellipsis_excluded_length = max_length_ - actual_ellipsis_length;

    return utf8FitStrLength(str_, ellipsis_excluded_length) + ellipsis_;
}

std::string util::timeTextFromSeconds(const std::chrono::seconds seconds_, bool ellipsis_)
{
    using namespace std::chrono_literals;
    const auto seconds = seconds_.count() % 60;
    const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(seconds_).count() % 60;
    const auto hours = std::chrono::duration_cast<std::chrono::hours>(seconds_).count() % 24;
    const auto days = std::chrono::duration_cast<std::chrono::days>(seconds_).count() % 30;
    const auto months = std::chrono::duration_cast<std::chrono::months>(seconds_).count() % 12;
    const auto years = std::chrono::duration_cast<std::chrono::years>(seconds_).count();

    if (years > 0) {
        if (ellipsis_) return std::format("{:02}Y{:02}M", years, months);
        return std::format("{:02}Y{:02}M{:02}D{:02}h{:02}m{:02}s", years, months, days, hours, minutes, seconds);
    }
    if (months > 0) {
        if (ellipsis_) return std::format("{:02}M{:02}D", months, days);
        return std::format("{:02}M{:02}D{:02}h{:02}m{:02}s", months, days, hours, minutes, seconds);
    }
    if (days > 0) {
        if (ellipsis_) return std::format("{:02}D{:02}h", days, hours);
        return std::format("{:02}D{:02}h{:02}m{:02}s", days, hours, minutes, seconds);
    }
    if (hours > 0) {
        if (ellipsis_) return std::format("{:02}h{:02}m", hours, minutes);
        return std::format("{:02}h{:02}m{:02}s", hours, minutes, seconds);
    }
    return std::format("{:02}m{:02}s", minutes, seconds);
}
