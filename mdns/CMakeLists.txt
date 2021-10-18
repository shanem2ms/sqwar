cmake_minimum_required(VERSION 3.0)
project(mdns VERSION 1.2.0)

set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# Set the output of the libraries and executables.
add_library(mdns STATIC mdns.c)