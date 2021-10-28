find_package(pybind11 REQUIRED)

pybind11_add_module(log_pybind
    log.cc
)

set_target_properties(log_pybind PROPERTIES 
    OUTPUT_NAME "log"
)

target_link_libraries(log_pybind PUBLIC log)

install(TARGETS log_pybind DESTINATION ${CMAKE_INSTALL_PREFIX}/python/lsst/log/log)
