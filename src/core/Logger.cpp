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

#include "Logger.h"
#include <chrono>
#include <format>
#include <sqlite3.h>

constexpr size_t rotate_count = 5;
constexpr size_t max_log_size = 1 * 1024 * 1024;

void Logger::initialize()
{
    std::call_once(_initialized, _initialize);
}


void Logger::log(const std::string& msg_, const std::string& log_level_, const std::string& reporter_) noexcept
{
    try {
        std::lock_guard lock(_mtx);
        if (_log_level_map.contains(log_level_) && _log_level_map.at(log_level_) < log_level) return;
        if (!_ensureOpenLogFile()) {
            _success_prev_logging = false;
            return;
        }
        _rotate();
        std::chrono::system_clock::time_point time = std::chrono::system_clock::now();
        _out << std::format("{:%c}\t", time);
        if (!log_level_.empty()) _out << "[" << log_level_ << "] ";
        if (!reporter_.empty()) _out << "<Reporter: " << reporter_ << "> ";
        _out << msg_ << "\n";
    }
    catch (std::exception& _) { _success_prev_logging = false; }
}

void Logger::debug(const std::string& msg_, const std::string& reporter_) noexcept { log(msg_, "DEBUG", reporter_); }

void Logger::info(const std::string& msg_, const std::string& reporter_) noexcept { log(msg_, "INFO", reporter_); }

void Logger::warning(const std::string& msg_, const std::string& reporter_) noexcept
{
    log(msg_, "WARNING", reporter_);
}

void Logger::error(const std::string& msg_, const std::string& reporter_) noexcept { log(msg_, "ERROR", reporter_); }

void Logger::critical(const std::string& msg_, const std::string& reporter_) noexcept
{
    log(msg_, "CRITICAL", reporter_);
}

void Logger::note(const std::string& msg_, const std::string& reporter_) noexcept { log(msg_, "NOTE", reporter_); }

void Logger::sqliteLoggingCallback([[maybe_unused]] void* pArg, int iErrCode, const char* zMsg) noexcept
{
    error(std::format("({}) {}", iErrCode, std::string(zMsg)), "SQLite");
}

bool Logger::isSuccessPrevLogging() { return _success_prev_logging; }

void Logger::updateLogLevelLabel(const std::string& label_, LogLevel level_)
{
    if (label_.empty()) return;
    if (_log_level_map.contains(label_)) { _log_level_map.at(label_) = level_; }
    else { _log_level_map.try_emplace(label_, level_); }
}

void Logger::setLogFilePath(const std::string& log_file_path_)
{
    std::lock_guard lock(_mtx);
    if (_out.is_open())
        _out.close();
    _log_file_path = log_file_path_;
}

Logger::LogLevel Logger::log_level = LogLevel::INFO;

std::unordered_map<std::string, Logger::LogLevel> Logger::_log_level_map{
    {"DEBUG", LogLevel::DEBUG},
    {"INFO", LogLevel::INFO},
    {"WARNING", LogLevel::WARNING},
    {"ERROR", LogLevel::ERROR},
    {"CRITICAL", LogLevel::CRITICAL},
    {"NOTE", LogLevel::INFO}
};

void Logger::_rotate()
{
    std::error_code err;
    if (!_out.is_open() || _out.fail() || _out.tellp() < max_log_size) { return; }
    _out.close();
    std::filesystem::remove(_getRotatePath(rotate_count), err);
    for (int i = rotate_count - 1; i >= 1; i--) {
        if (std::filesystem::exists(_getRotatePath(i), err)) {
            std::filesystem::rename(_getRotatePath(i), _getRotatePath(i + 1), err);
        }
    }
    std::filesystem::rename(_log_file_path, _getRotatePath(1), err);
    _openLogFile();
}

void Logger::_openLogFile() { _out.open(_log_file_path, std::ios::out | std::ios::in | std::ios::app); }

bool Logger::_ensureOpenLogFile()
{
    if (!_out.is_open()) { _openLogFile(); }
    else return true;
    return _out.is_open();
}

std::filesystem::path Logger::_getRotatePath(const size_t rotateNumber)
{
    return _log_file_path.parent_path() / (_log_file_path.stem().string() + "." + std::to_string(rotateNumber) +  _log_file_path.extension().string());
}

void Logger::_initialize() { sqlite3_config(SQLITE_CONFIG_LOG, sqliteLoggingCallback, nullptr); }

std::ofstream Logger::_out;
std::once_flag Logger::_initialized;
std::mutex Logger::_mtx;
bool Logger::_success_prev_logging = true;
std::filesystem::path Logger::_log_file_path{util::getDataPath("program.log")};
