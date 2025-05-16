cmake_minimum_required(VERSION 3.31)
project(todo-and-timecard-tui)

include(FetchContent)

FetchContent_Declare(ftxui
        GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
        GIT_TAG v6.1.9
)

FetchContent_MakeAvailable(ftxui)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(dependencies/sqlite3)

add_executable(todo-and-timecard-tui src/main.cpp
        src/core/TodoAndTimeCardApp.cpp
        src/core/TodoAndTimeCardApp.h
        src/page/TodoListPage.cpp
        src/page/TodoListPage.h
        src/core/DBManager.cpp
        src/core/DBManager.h
        src/utilities/Utilities.cpp
        src/utilities/Utilities.h
        src/resource.h
        src/core/Logger.cpp
        src/core/Logger.h
        src/components/Console.cpp
        src/components/Console.h
        src/components/TaskListView.cpp
        src/components/TaskListView.h
        src/components/custom_menu_entry.cpp
        src/components/custom_menu_entry.h
        src/utilities/DurationTimer.cpp
        src/utilities/DurationTimer.h
)

target_link_libraries(todo-and-timecard-tui PRIVATE ftxui::dom ftxui::component sqlite3)
target_include_directories(todo-and-timecard-tui PRIVATE ${ftxui_SOURCE_DIR}/include)
