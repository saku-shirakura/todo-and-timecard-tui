cmake_minimum_required(VERSION 3.31)
project(todo_and_timecard_tui)

include(FetchContent)

FetchContent_Declare(ftxui
        GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
        GIT_TAG v6.1.9
)

FetchContent_MakeAvailable(ftxui)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(lib/sqlite3)

add_executable(todo_and_timecard_tui src/main.cpp
        src/core/TodoAndTimeCardApp.cpp
        src/core/TodoAndTimeCardApp.h
        src/page/TodoListPage.cpp
        src/page/TodoListPage.h
        src/core/DBManager.cpp
        src/core/DBManager.h
        src/page/PageBase.cpp
        src/page/PageBase.h
        src/utilities/Utilities.cpp
        src/utilities/Utilities.h
        src/resource.h
        src/core/Logger.cpp
        src/core/Logger.h
        src/lib/git-tui/scroller.hpp
        src/lib/git-tui/scroller.cpp
)

target_link_libraries(todo_and_timecard_tui PRIVATE ftxui::dom ftxui::component sqlite3)
target_include_directories(todo_and_timecard_tui PRIVATE ${ftxui_SOURCE_DIR}/include)
