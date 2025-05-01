cmake_minimum_required(VERSION 3.31)
project(todo_and_timecard_tui)

include(FetchContent)

FetchContent_Declare(ftxui
        GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
        GIT_TAG v6.1.8
)

FetchContent_MakeAvailable(ftxui)

set(CMAKE_CXX_STANDARD 20)

add_executable(todo_and_timecard_tui src/main.cpp
        src/core/TodoAndTimeCardApp.cpp
        src/core/TodoAndTimeCardApp.h
        src/page/TodoListPage.cpp
        src/page/TodoListPage.h
)

target_link_libraries(todo_and_timecard_tui PRIVATE ftxui::dom ftxui::component)
target_include_directories(todo_and_timecard_tui PRIVATE ${ftxui_SOURCE_DIR}/include)