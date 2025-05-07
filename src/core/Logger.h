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
 * @file Logger.h
 * @date 25/05/05
 * @brief ファイルの説明
 * @details ファイルの詳細
 * @author saku shirakura (saku@sakushira.com)
 */


#ifndef LOGGER_H
#define LOGGER_H
#include <format>
#include <fstream>
#include <mutex>
#include <sqlite3.h>
#include <string>
#include <unordered_map>

#include "../utilities/Utilities.h"


/**
 * @brief ここにクラスの説明
 * @details ここにクラスの詳細な説明
 * @since
 */
class Logger {
public:
    /**
     * @brief sqlite3のロギングコールバックを設定するなど、ロガーの初期設定を行います。main()の先頭で呼び出してください。
     */
    static void initialize();

    enum class LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    /**
     * @brief 時刻とmsgからなるログメッセージをログファイルに出力します。
     * @note ファイルが開けなかった場合、処理は行われません。
     * @param msg_ ログメッセージ
     * @param log_level_ ログレベル(任意) 指定しない場合省略されます。
     * @param reporter_
     */
    static void log(const std::string& msg_, const std::string& log_level_ = "", const std::string& reporter_ = "") noexcept;

    // 基本ログレベル
    static void debug(const std::string& msg_, const std::string& reporter_ = "Anonymous") noexcept;
    static void info(const std::string& msg_, const std::string& reporter_ = "Anonymous") noexcept;
    static void warning(const std::string& msg_, const std::string& reporter_ = "Anonymous") noexcept;
    static void error(const std::string& msg_, const std::string& reporter_ = "Anonymous") noexcept;
    static void critical(const std::string& msg_, const std::string& reporter_ = "Anonymous") noexcept;

    // LogLevel::INFO
    static void note(const std::string& msg_, const std::string& reporter_ = "Anonymous") noexcept;

    // LogLevel::ERROR
    static void sqliteLoggingCallback(void* pArg, int iErrCode, const char* zMsg) noexcept;

    static bool isSuccessPrevLogging();

    static void updateLogLevelLabel(const std::string& label_, LogLevel level_);

    static void setLogFilePath(const std::string& log_file_path_);

    static LogLevel log_level;
private:
    static std::unordered_map<std::string, LogLevel> _log_level_map;

    /**
     * @brief ログローテーションを実行します。
     */
    static void _rotate();
    static void _openLogFile();

    static bool _ensureOpenLogFile();

    static std::filesystem::path _getRotatePath(size_t rotateNumber);

    static std::ofstream _out;
    static bool _initialized;
    static std::mutex _mtx;
    static bool _success_prev_logging;
    static std::filesystem::path _log_file_path;
};


#endif //LOGGER_H
