cmake_minimum_required(VERSION 3.20)
project(project)

set(CMAKE_CXX_STANDARD 20)

add_executable(
        project main.cpp
        src/query/initialize.cpp
        src/query/inverted_list.cpp
        src/query/query.cpp
        src/indexing/index_builder.cpp
        src/indexing/trec_reader.cpp
        src/indexing/structs.h)
