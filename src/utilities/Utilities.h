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
 * @file Utilities.h
 * @date 25/05/03
 * @brief ファイルの説明
 * @details ファイルの詳細
 * @author saku shirakura (saku@sakushira.com)
 */


#ifndef UTILITIES_H
#define UTILITIES_H
#include <filesystem>
#include <string>

namespace util {
    /**
     * @brief getenv_sをC++で扱いやすい形式にしたラッパー
     * @exception std::runtime_error メモリを割り当てられませんでした。
     * @exception std::runtime_error 環境変数を取得できませんでした。
     * @param variable_name_ 取得したい環境変数名
     * @param default_value_
     * @return 環境変数から取得した値。
     */
    std::string getEnv(const std::string& variable_name_, const std::string& default_value_ = "");

    /**
     * @brief アプリデータディレクトリ及び、アプリデータファイルのパスを取得します。
     * @param filename_ デフォルトは空文字列。
     * @return アプリデータディレクトリに`filename_`を結合したパス。エラー時には空文字列が帰ります。
     */
    std::string getDataPath(const std::string& filename_ = "");

}

#endif //UTILITIES_H
