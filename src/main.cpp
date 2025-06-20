// MIT License
//
// Copyright (c) saku shirakura <saku@sakushira.com>
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


#include <iostream>

#include "resource.h"
#include "core/DBManager.h"
#include "core/Logger.h"
#include "core/TodoAndTimeCardApp.h"

class ApplicationStartEndLogger {
public:
    ApplicationStartEndLogger() { Logger::info("start application.", "main"); }

    ~ApplicationStartEndLogger() { Logger::info("finish application.", "main"); }
};

void startup()
{
#ifndef NDEBUG
    Logger::setLogFilePath("dev.log");
    if (!core::db::DBManager::setDBFile("dev.sqlite")) {
        Logger::error("Failed to change database file path.", "main");
    }
#endif
    Logger::loadFromSettings();
    Logger::initialize();
    ApplicationStartEndLogger logger;
    core::TodoAndTimeCardApp::execute();
}

bool executeOption(std::vector<std::string> args)
{
    const std::vector<std::string> options{"--version", "--v", "--help", "--license", "--notice"};
    for (const auto& option : options) {
        if (std::ranges::find(args, option) != args.end()) {
            if (option == "--version" || option == "-v") {
                std::cout << std::string(F_VERSION_, SIZE_VERSION_);
#ifndef NDEBUG
                std::cout << ".debug";
#endif
                std::cout << std::endl;
            }
            else if (option == "--help") {
                std::cout << R"(Usage:
    todo-and-timecard-tui           : Start the software.
    todo-and-timecard-tui --version : Show the software version.
    todo-and-timecard-tui --license : Show the license.
    todo-and-timecard-tui --notice  : Show the contents of the Notice file.)"
                    << std::endl;
            }
            else if (option == "--license") { std::cout << std::string(F_LICENSE_, SIZE_LICENSE_) << std::endl; }
            else if (option == "--notice") { std::cout << std::string(F_NOTICE_, SIZE_NOTICE_) << std::endl; }
            return true;
        }
    }
    return false;
}

// ReSharper disable once CppDFAConstantFunctionResult
int main(const int argc, char** argv)
{
    const std::vector<std::string> args(argv, argv + argc);
    if (executeOption(args)) return 0;
    startup();
    return 0;
}
