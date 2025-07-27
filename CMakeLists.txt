cmake_minimum_required(VERSION 3.12)
project(log)

enable_testing()

add_subdirectory(python/lsst/log)
add_subdirectory(src)
add_subdirectory(tests)
