# 定义一个SRC_LIST变量，包含该目录下所有的源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_SRC_LIST)
aux_source_directory(./model MODEL_SRC_LIST)
aux_source_directory(./redis REDIS_SRC_LIST)
# 指定生成可执行文件
add_executable(ChatServer ${SRC_LIST} ${DB_SRC_LIST} ${MODEL_SRC_LIST} ${REDIS_SRC_LIST})
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient hiredis pthread)
