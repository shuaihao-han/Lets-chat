# 定义SRC_LIST变量,包含该目录下的所有源文件
aux_source_directory(. SRC_LIST)

# 指定生成 客户端的可执行文件 
add_executable(ChatClient ${SRC_LIST})

# 指定可执行文件连接时需要依赖的库文件
target_link_libraries(ChatClient pthread)