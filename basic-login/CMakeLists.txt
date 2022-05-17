cmake_minimum_required(VERSION 3.15)
#SET (CMAKE_CXX_COMPILER "/usr/bin/clang++" CACHE STRING "C++ compiler" FORCE)
project( basic-login )
# Enable C++17
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Threads REQUIRED)
add_executable( basic-login basic-login.cpp )
target_link_libraries(basic-login Threads::Threads)
add_custom_target(run)
add_custom_command( TARGET run
	POST_BUILD COMMAND ./basic-login)
