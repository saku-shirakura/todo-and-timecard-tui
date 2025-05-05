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

void Logger::initialize()
{
    if (_initialized)
        return;
    sqlite3_config(SQLITE_CONFIG_LOG, sqliteLoggingCallback, nullptr);
    _initialized = true;
}

void Logger::log(const std::string& msg_, const std::string& log_level_) noexcept
{
    try {
        std::lock_guard lock(_mtx);
        if (!_ensureOpenLogFile()) {
            _success_prev_logging = false;
            return;
        }
        std::chrono::system_clock::time_point time = std::chrono::system_clock::now();
        _out << std::format("{:%c}\t", time);
        if (!log_level_.empty()) _out << "[" << log_level_ << "] ";
        _out << msg_ << "\n";
    }
    catch (std::exception& _) {
        _success_prev_logging = false;
    }
}

void Logger::debug(const std::string& msg_) noexcept { log(msg_, "DEBUG"); }

void Logger::info(const std::string& msg_) noexcept { log(msg_, "INFO"); }

void Logger::warning(const std::string& msg_) noexcept { log(msg_, "WARNING"); }

void Logger::error(const std::string& msg_) noexcept { log(msg_, "ERROR"); }

void Logger::critical(const std::string& msg_) noexcept { log(msg_, "CRITICAL"); }

void Logger::sqliteLoggingCallback([[maybe_unused]] void* pArg, int iErrCode, const char* zMsg) noexcept
{
    error(std::format("From Sqlite Logger: ({}) {}", iErrCode, std::string(zMsg)));
}

bool Logger::isSuccessPrevLogging()
{
    return _success_prev_logging;
}

void Logger::_openLogFile()
{
    _out.open(util::getDataPath("program.log"), std::ios::out | std::ios::in | std::ios::app);
}

bool Logger::_ensureOpenLogFile()
{
    if (!_out.is_open()) { _openLogFile(); }
    else return true;
    return _out.is_open();
}

std::ofstream Logger::_out;
bool Logger::_initialized = false;
std::mutex Logger::_mtx;
bool Logger::_success_prev_logging = true;
