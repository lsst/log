find_package(Boost REQUIRED COMPONENTS
    unit_test_framework REQUIRED
)

find_package(Threads REQUIRED)

add_executable(testLog testLog.cc)

target_link_libraries(testLog PUBLIC
    log
    Boost::unit_test_framework
    Threads::Threads
)

add_test(NAME testLog COMMAND testLog)
