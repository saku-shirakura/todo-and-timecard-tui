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

    /**
     * @brief utf8の文字数をカウントする。
     * @return Ascii文字は1、それ以外は2として扱われます。
     */
    size_t countUtf8Character(const std::string& str_);

    /**
     * @brief utf-8のバイト数を判定します。その際、文字の一部を構成する物については0を返し、不正な文字には-1を返します。
     * @param c utf-8の1バイト
     * @returns -1の場合、その値は無効です。
     * @returns 0の場合、その値は文字の一部です。
     * @returns 1-4の整数の場合、そのバイトと続く`n - 1`バイト分が1つの文字を表現しています。
     */
    int utf8CharSize(unsigned char c);

    /**
     * @brief utf-8文字列の長さをlength_に可能な限り近づけます。(マルチバイト文字は2文字、Asciiは1文字)
     * @param str_ utf8の文字列
     * @param length_ 近づける長さ。
     * @return 戻り値は、str_の長さがlength_以上であれば、文字列の長さはlength_と等しいかlength_ - 1になります。
     */
    std::string utf8FitStrLength(const std::string& str_, size_t length_);

    /**
     * @brief 文字列が`max_length`を超える場合省略します。
     * @param str_ 対象文字列
     * @param max_length_ 最大文字長 (マルチバイト文字は2文字、Asciiは1文字)
     * @param ellipsis_ 省略記号(デフォルトは" ...")
     * @return 文字数が`max_length_`を超えている場合省略したものを返します。
     */
    std::string ellipsisString(const std::string& str_, size_t max_length_, const std::string& ellipsis_ = " ...");

    /**
     * @brief 秒数を時間を表すテキストに変換します。
     * @param seconds_ 秒数
     * @param ellipsis_ このフラグが有効な倍上位2桁のみをテキストに出力します。
     * @returns ellipsis_がtrueなら、01Y02Mのような時間を表すテキスト。
     * @returns そうでないなら、01Y02M03D00h01m02sのような時間を表すテキスト
     */
    std::string timeTextFromSeconds(std::chrono::seconds seconds_, bool ellipsis_ = false);
}

#endif //UTILITIES_H
