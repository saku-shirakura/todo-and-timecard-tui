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


std::string util::getEnv(const std::string& variable_name_, const std::string& default_value_)
{
    const char *env = std::getenv(variable_name_.c_str());
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
