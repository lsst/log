add_library(log SHARED)

target_compile_features(log PRIVATE
    cxx_std_17
)

set_target_properties(log PROPERTIES 
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(log PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_sources(log PRIVATE
    Log.cc
    lwpID.cc
    lwpID.h
)

target_link_libraries(log PUBLIC
    log4cxx
)

install(TARGETS log)
