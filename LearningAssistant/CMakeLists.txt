cmake_minimum_required(VERSION 3.16)
project(LearningAssistant VERSION 1.0.0 LANGUAGES CXX)

# ============================================================
# C++ 标准设置
# ============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# 编译选项
# ============================================================
if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ============================================================
# 功能开关 —— 控制各模块是否编译
# ============================================================
option(ENABLE_QT       "Build Qt GUI frontend"          OFF)
option(ENABLE_OPENCV   "Build OpenCV OCR module"        OFF)
option(ENABLE_ONNX     "Build ONNX Runtime AI module"   OFF)
option(ENABLE_CURL     "Build libcurl HTTP module"      ON)
option(ENABLE_SQLITE   "Build SQLite database module"   ON)
option(ENABLE_PLUGINS  "Build dynamic plugin system"    ON)
option(ENABLE_THREADS  "Build multithreading module"    ON)

# ============================================================
# 第三方头文件库
# ============================================================
add_subdirectory(third_party)

# ============================================================
# 条件查找外部依赖
# ============================================================
if(ENABLE_QT)
    find_package(Qt6 COMPONENTS Widgets Core Gui QUIET)
    if(NOT Qt6_FOUND)
        find_package(Qt5 5.15 COMPONENTS Widgets Core Gui REQUIRED)
        set(QT_LIBS Qt5::Widgets Qt5::Core Qt5::Gui)
    else()
        set(QT_LIBS Qt6::Widgets Qt6::Core Qt6::Gui)
    endif()
    add_definitions(-DHAS_QT)
endif()

if(ENABLE_OPENCV)
    find_package(OpenCV QUIET)
    if(OpenCV_FOUND)
        add_definitions(-DHAS_OPENCV)
    else()
        message(WARNING "OpenCV not found, OCR module disabled")
        set(ENABLE_OPENCV OFF)
    endif()
endif()

if(ENABLE_ONNX)
    find_package(onnxruntime QUIET)
    if(onnxruntime_FOUND)
        add_definitions(-DHAS_ONNX)
    else()
        message(WARNING "ONNX Runtime not found, AI module disabled")
        set(ENABLE_ONNX OFF)
    endif()
endif()

if(ENABLE_CURL)
    find_package(CURL QUIET)
    if(NOT CURL_FOUND)
        # Windows 回退: 使用 WinHTTP
        message(STATUS "libcurl not found, using WinHTTP fallback")
    endif()
endif()

# ============================================================
# 核心库（始终编译）
# ============================================================
add_library(la_core STATIC
    src/core/app_config.cpp
    src/core/logger.cpp
    # src/core/error_handler.cpp  (header-only)
)
target_include_directories(la_core PUBLIC src)
target_link_libraries(la_core PUBLIC la_third_party)

# ============================================================
# 条件编译模块
# ============================================================
if(ENABLE_SQLITE)
    add_library(la_database STATIC
        src/database/sqlite_wrapper.cpp
        src/database/note_repository.cpp
    )
    target_link_libraries(la_database PUBLIC la_core sqlite3)
    target_include_directories(la_database PUBLIC src)
endif()

if(ENABLE_THREADS)
    add_library(la_threading STATIC
        src/threading/thread_pool.cpp
        src/threading/task_manager.cpp
    )
    target_link_libraries(la_threading PUBLIC la_core)
    target_include_directories(la_threading PUBLIC src)
    if(WIN32)
        target_link_libraries(la_threading PRIVATE pthread)
    endif()
endif()

if(ENABLE_CURL)
    add_library(la_network STATIC
        src/network/http_client.cpp
        src/network/json_parser.cpp
    )
    target_link_libraries(la_network PUBLIC la_core)
    target_include_directories(la_network PUBLIC src)
    if(CURL_FOUND)
        target_link_libraries(la_network PUBLIC CURL::libcurl)
    endif()
endif()

if(ENABLE_OPENCV)
    add_library(la_ocr STATIC
        src/ocr/screen_capture.cpp
        src/ocr/image_processor.cpp
    )
    target_link_libraries(la_ocr PUBLIC la_core ${OpenCV_LIBS})
    target_include_directories(la_ocr PUBLIC src)
endif()

if(ENABLE_ONNX)
    add_library(la_ai STATIC
        src/ai/onnx_inference.cpp
    )
    target_link_libraries(la_ai PUBLIC la_core onnxruntime)
    target_include_directories(la_ai PUBLIC src)
endif()

if(ENABLE_PLUGINS)
    add_library(la_plugins STATIC
        src/plugins/plugin_manager.cpp
    )
    target_link_libraries(la_plugins PUBLIC la_core)
    target_include_directories(la_plugins PUBLIC src)
endif()

# ============================================================
# 主程序
# ============================================================
add_executable(LearningAssistant
    src/main.cpp
)

# 链接所有启用的模块
target_link_libraries(LearningAssistant PRIVATE la_core)
if(ENABLE_SQLITE)
    target_link_libraries(LearningAssistant PRIVATE la_database)
endif()
if(ENABLE_THREADS)
    target_link_libraries(LearningAssistant PRIVATE la_threading)
endif()
if(ENABLE_CURL)
    target_link_libraries(LearningAssistant PRIVATE la_network)
endif()
if(ENABLE_OPENCV)
    target_link_libraries(LearningAssistant PRIVATE la_ocr)
endif()
if(ENABLE_ONNX)
    target_link_libraries(LearningAssistant PRIVATE la_ai)
endif()
if(ENABLE_PLUGINS)
    target_link_libraries(LearningAssistant PRIVATE la_plugins)
endif()

# ============================================================
# Qt GUI 可执行文件（独立编译）
# ============================================================
if(ENABLE_QT)
    add_executable(LearningAssistantGUI
        src/gui/main_window.cpp
        src/gui/note_editor.cpp
        src/gui/settings_dialog.cpp
        src/gui/main_gui.cpp
    )
    target_link_libraries(LearningAssistantGUI PRIVATE
        la_core la_database la_threading la_network
        ${QT_LIBS}
    )
    target_include_directories(LearningAssistantGUI PRIVATE src)
    set_target_properties(LearningAssistantGUI PROPERTIES
        WIN32_EXECUTABLE TRUE
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
    )
endif()

# ============================================================
# 示例插件
# ============================================================
if(ENABLE_PLUGINS)
    add_subdirectory(plugins)
endif()

# ============================================================
# 安装规则
# ============================================================
install(TARGETS LearningAssistant RUNTIME DESTINATION bin)
install(DIRECTORY plugins/ DESTINATION lib/plugins
    FILES_MATCHING PATTERN "*.dll" PATTERN "*.so")
message(STATUS "=== LearningAssistant Configuration ===")
message(STATUS "  C++ Standard : ${CMAKE_CXX_STANDARD}")
message(STATUS "  Qt           : ${ENABLE_QT}")
message(STATUS "  OpenCV       : ${ENABLE_OPENCV}")
message(STATUS "  ONNX Runtime : ${ENABLE_ONNX}")
message(STATUS "  libcurl      : ${ENABLE_CURL}")
message(STATUS "  SQLite       : ${ENABLE_SQLITE}")
message(STATUS "  Plugins      : ${ENABLE_PLUGINS}")
message(STATUS "======================================")
