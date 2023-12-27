cmake_minimum_required(VERSION 3.0)
project(ereactor)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -Wall -fPIC -Wno-deprecated")

file(GLOB_RECURSE SOURCES src/*.cc)
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/udp_server.cc")
#list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/udp_server.cc")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(server 
        server.cc
        ${SOURCES}
    )

add_executable(client 
        client.cc
        ${SOURCES}
    )

target_link_libraries(
    client
    -lpthread
)


target_link_libraries(
    server
    -lpthread
)


set_target_properties(server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set_target_properties(client PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# 将头文件复制到输出目录
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test/myconf.ini ${CMAKE_CURRENT_SOURCE_DIR}/bin COPYONLY)



# Add any other dependencies and linker flags if necessary
# target_link_libraries(ereactor <dependency_library>)

# Add any executable targets if necessary
# add_executable(my_executable src/main.cpp)
# target_link_libraries(my_executable ereactor)

# Optionally, you can install the library
# install(TARGETS ereactor DESTINATION lib)
# install(DIRECTORY include DESTINATION include)

# Optionally, clean build files
# add_custom_target(clean-all COMMAND ${CMAKE_MAKE_PROGRAM} clean && rm -rf bin lib)