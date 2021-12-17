cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
#vcpkg
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
  CACHE STRING "Vcpkg toolchain file")

project(ServerlessSQLite VERSION 0.1.0)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
if (APPLE)
    # todo
else ()
    set(CMAKE_EXE_LINKER_FLAGS "-static -pthread -lrt -ldl -lstdc++")
endif ()

# dependencies
find_package(glog CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
# find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(hiredis CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# executable: query
file(GLOB QUERY_SRC_FILES src/query/*.cpp)
add_executable(query ${QUERY_SRC_FILES})
target_link_libraries(query PRIVATE unofficial::sqlite3::sqlite3)
target_link_libraries(query PRIVATE glog::glog)
# target_link_libraries(query PRIVATE Boost::boost Boost::filesystem)
target_link_libraries(query PRIVATE gRPC::gpr gRPC::upb gRPC::grpc gRPC::grpc++)

# executable: store
file(GLOB STORE_SRC_FILES src/store/*.cpp)
add_executable(store ${STORE_SRC_FILES})
target_link_libraries(store PRIVATE glog::glog)
target_link_libraries(store PRIVATE hiredis::hiredis)
target_link_libraries(store PRIVATE gRPC::gpr gRPC::upb gRPC::grpc gRPC::grpc++)
