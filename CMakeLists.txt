cmake_minimum_required(VERSION 3.5)

project(gentmplhash LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(gentmplhash
    main.cpp
    md5/md5.h
    md5/md5.cpp
    tinyxml2/tinyxml2.h
    tinyxml2/tinyxml2.cpp
)
