
include_directories (../../../../third_party/include)
include_directories (../../../../third_party/include/db297)

include_directories (.)
include_directories (../src)

link_directories (../../../../app/DB2ReadLogServer/lib/db297)

aux_source_directory (../../server/src SERVER_SRC_SRCS)
aux_source_directory (../../share SHARE_SRCS)

add_definitions(-DV9_CONFIG_SERVER_ADDRESS)

add_executable(db209_src_srv ${SERVER_SRC_SRCS} ${SHARE_SRCS} dblogconn.cpp utilemb.C db2_log_version.cpp lri_recorder.cpp)

target_link_libraries(db209_src_srv
-static-libgcc -static-libstdc++ libstdc++fs.a -lsqlite3
db2
DB2PlugGrpcProto pthread
)

if(BUILD_CENTOS7)
add_custom_command(TARGET db209_src_srv
POST_BUILD
	COMMAND mkdir -m775 ${PROJECT_SOURCE_DIR}/../../app/DB2ReadLogServer/bin/centos7/v9/ -p
	COMMAND cp -a db209_src_srv ${PROJECT_SOURCE_DIR}/../../app/DB2ReadLogServer/bin/centos7/v9/
)
endif()

if(BUILD_CENTOS6)
add_custom_command(TARGET db209_src_srv
POST_BUILD
	COMMAND mkdir -m775 ${PROJECT_SOURCE_DIR}/../../app/DB2ReadLogServer/bin/centos6/v9/ -p
	COMMAND cp -a db209_src_srv ${PROJECT_SOURCE_DIR}/../../app/DB2ReadLogServer/bin/centos6/v9/
)
endif()

if(BUILD_UBUNTU184)
add_custom_command(TARGET db209_src_srv
POST_BUILD
	COMMAND mkdir -m775 ${PROJECT_SOURCE_DIR}/../../app/DB2ReadLogServer/bin/ubuntu184/v9/ -p
	COMMAND cp -a db209_src_srv ${PROJECT_SOURCE_DIR}/../../app/DB2ReadLogServer/bin/ubuntu184/v9/
)
endif()
