# 插件构建
# 每个插件编译为独立的动态库

# --- Hello 插件 ---
add_library(hello_plugin SHARED
    hello_plugin/hello_plugin.cpp
)
target_include_directories(hello_plugin PRIVATE ${CMAKE_SOURCE_DIR}/src)
set_target_properties(hello_plugin PROPERTIES
    PREFIX ""  # Windows: 去掉 lib 前缀
)
# 将编译产物放到 plugins/ 目录
set_target_properties(hello_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
)
