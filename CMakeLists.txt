cmake_minimum_required(VERSION 3.0.0)
add_compile_options(-Wall -Wextra -pedantic -Werror)
project(search-server)

add_executable(search-server 
    search-server/main.cpp
    search-server/document.cpp
    search-server/process_queries.cpp
    search-server/read_input_functions.cpp
    search-server/request_queue.cpp
    search-server/search_server.cpp
    search-server/string_processing.cpp    
    search-server/test_example_functions.cpp
    )
