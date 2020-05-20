# CMakeList.txt: SoftlyNetTest 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

project ("SoftlyNetTest")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-D__STDC_CONSTANT_MACROS")


#======================= OpenCv =============================
find_package(OpenCV REQUIRED)

message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")
#======================= OpenCv =============================

if(MSVC)
    SET(CMAKE_CXX_FLAGS "/EHsc")  # deal with MSVC Warning C4530
endif(MSVC)

if(NOT SPDLOG_ACTIVE_LEVEL)
    set(SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG)
endif()
if(NOT LOG_FILE_NAME)
    set(LOG_FILE_NAME "appliaction.log")
endif()

 configure_file(
       "config.h.in"
       "config.h"
)

 include_directories(${CMAKE_CURRENT_BINARY_DIR})

# 将源代码添加到此项目的可执行文件。
add_executable (${PROJECT_NAME}
 "src/Server.cpp" 
 "src/Client.cpp")

include_directories(${OpenCV_INCLUDE_DIRS})



target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${OpenCV_LIBS}
        )

# TODO: 如有需要，请添加测试并安装目标。
