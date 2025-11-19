set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
cmake_minimum_required(VERSION 3.5.1)

project(DB2PlugInDataSource C CXX)

find_package(Threads REQUIRED)

set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
if(CMAKE_CROSSCOMPILING)
  find_program(_PROTOBUF_PROTOC protoc)
else()
  set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()

# Find gRPC installation
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(_GRPC_GRPCPP gRPC::grpc++)
if(CMAKE_CROSSCOMPILING)
  find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
  set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()

set(db2_plug_proto_srcs "${PROJECT_SOURCE_DIR}/proto/DB2PlugInDataSource.pb.cc")
set(db2_plug_proto_hdrs "${PROJECT_SOURCE_DIR}/proto/DB2PlugInDataSource.pb.h")
set(db2_plug_grpc_srcs "${PROJECT_SOURCE_DIR}/proto/DB2PlugInDataSource.grpc.pb.cc")
set(db2_plug_grpc_hdrs "${PROJECT_SOURCE_DIR}/proto/DB2PlugInDataSource.grpc.pb.h")

if(NOT SKIP_BUILD_PROTO)
  get_filename_component(db2_plug_proto "protos/DB2PlugInDataSource.proto" ABSOLUTE)
  get_filename_component(db2_plug_proto_path "${db2_plug_proto}" PATH)

  add_custom_command(
        OUTPUT "${db2_plug_proto_srcs}" "${db2_plug_proto_hdrs}" "${db2_plug_grpc_srcs}" "${db2_plug_grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${PROJECT_SOURCE_DIR}/proto"
          --cpp_out "${PROJECT_SOURCE_DIR}/proto"
          -I "${db2_plug_proto_path}"
          --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
          "${db2_plug_proto}"
        DEPENDS "${db2_plug_proto}"
        )
endif()

include_directories (proto)

add_library(DB2PlugGrpcProto
  ${db2_plug_proto_srcs}
  ${db2_plug_proto_hdrs}
  ${db2_plug_grpc_srcs}
  ${db2_plug_grpc_hdrs}
)

target_link_libraries(DB2PlugGrpcProto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
)

execute_process(COMMAND mkdir -m775 ${PROJECT_SOURCE_DIR}/../../app/DB2ReadLogServer/lib/ -p)

if(NOT BUILD_CENTOS7 AND NOT BUILD_CENTOS6 AND NOT BUILD_UBUNTU184)
	SET(BUILD_CENTOS7 TRUE)
endif()

include_directories (share)
include_directories (proto)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -O0 -std=c++17 ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -std=c++17 ${CMAKE_CXX_FLAGS}")

execute_process(COMMAND mkdir -m775 ${PROJECT_SOURCE_DIR}/../../app/DB2ReadLogServer/lib/ -p)
#将第三方库，尤其是涉及到有软连接的库，全部放在lib.tgz里面去，平时不开启
execute_process(COMMAND tar -xvf  ${PROJECT_SOURCE_DIR}/../../third_party/lib/lib.tgz -C ${PROJECT_SOURCE_DIR}/../../app/DB2ReadLogServer/lib/)

#DEBUG
#add_definitions(-DSERVER_DEBUG)
#add_definitions(-DSERVER_SUBLOG_DEBUG)

include_directories (${PROJECT_SOURCE_DIR}/../../third_party/include)

aux_source_directory (share SHARE_SRCS1)
aux_source_directory (client CLIENT_SRCS)

add_executable(
    test_client
    ${SHARE_SRCS1}
    ${CLIENT_SRCS}
)

target_link_libraries(
    test_client
    DB2PlugGrpcProto pthread
    libstdc++fs.a
)

add_subdirectory(server/logv9)
add_subdirectory(server/logv10)



