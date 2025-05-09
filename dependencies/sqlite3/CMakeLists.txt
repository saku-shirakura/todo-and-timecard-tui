project(sqlite3)

add_library(sqlite3 src/sqlite3.c)

target_include_directories(sqlite3 PUBLIC include)
