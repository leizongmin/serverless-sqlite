cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

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

project(ServerlessSQLite VERSION 0.1.0)

# executable: query
file(GLOB QUERY_SRC_FILES src/query/*.cpp)
add_executable(query ${QUERY_SRC_FILES})
# library: sqlite3
find_package(unofficial-sqlite3 CONFIG REQUIRED)
target_link_libraries(query PRIVATE unofficial::sqlite3::sqlite3)
# library: glog
find_package(glog CONFIG REQUIRED)
target_link_libraries(query PRIVATE glog::glog)
# library: boost
# find_package(Boost REQUIRED COMPONENTS filesystem)
# target_link_libraries(query PRIVATE Boost::boost Boost::filesystem)
