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

#include <stdexcept>


std::string util::getEnv(const std::string& variableName)
{
    size_t required_size;

    // 環境変数のサイズを取得し、メモリを確保する。
    getenv_s(&required_size, nullptr, 0, variableName.c_str());
    const auto buff = static_cast<char*>(malloc(required_size));
    if (!buff) { throw std::runtime_error("Fatal: Failed to allocate memory."); }

    // 環境変数を取得する。
    if (const errno_t error_no = getenv_s(&required_size, buff, required_size, variableName.c_str())) {
        free(buff);
        throw std::runtime_error("getenv_s error. errno: " + std::to_string(error_no));
    }
    std::string result{buff, required_size - 1 /* ナル文字を含めない */};
    free(buff);
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
        path /= "com.sakushira.todo-and-timecard-tui";

        // ファイル名を加えたパス
        if (!filename_.empty()) { path /= std::filesystem::path{filename_}; }
        return {path.generic_string()};
    }
    catch (const std::exception&) { return ""; }
}
