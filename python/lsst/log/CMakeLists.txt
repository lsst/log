find_package(Python COMPONENTS Development Interpreter)
add_subdirectory(log)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_PREFIX}/python/lsst/log
    FILES_MATCHING PATTERN "*.py"
)

add_custom_target(log_version ALL
    ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/version.cmake
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/version.py
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/version.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/python/lsst/log
)
