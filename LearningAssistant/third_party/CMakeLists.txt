# 第三方依赖管理
# 使用 header-only 库，无需预编译

# --- nlohmann/json (header-only) ---
# 下载: https://github.com/nlohmann/json/releases
# 将 json.hpp 放到 third_party/json/ 目录下
add_library(la_third_party INTERFACE)
target_include_directories(la_third_party INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# --- SQLite3 合并版 ---
# 下载: https://www.sqlite.org/download.html (sqlite-amalgamation-*.zip)
# 将 sqlite3.c 和 sqlite3.h 放到 third_party/sqlite3/ 目录下
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sqlite3/sqlite3.c")
    add_library(sqlite3 STATIC sqlite3/sqlite3.c)
    target_include_directories(sqlite3 PUBLIC sqlite3)
    target_compile_definitions(sqlite3 PRIVATE
        SQLITE_THREADSAFE=1
        SQLITE_ENABLE_FTS5
        SQLITE_ENABLE_JSON1
    )
else()
    message(STATUS "sqlite3 amalgamation not found in third_party/sqlite3/")
    message(STATUS "  Download from: https://www.sqlite.org/download.html")
    message(STATUS "  Place sqlite3.c and sqlite3.h in third_party/sqlite3/")
endif()
