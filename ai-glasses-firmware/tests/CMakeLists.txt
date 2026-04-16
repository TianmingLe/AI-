add_library(catch2_mock STATIC /workspace/mocks/catch2/mock_main.cpp)
target_include_directories(catch2_mock PUBLIC /workspace/mocks/catch2)

add_executable(run_tests
    test_sanity.cpp
    test_http.cpp
    test_udp.cpp
    test_gps.cpp
    test_sensor_fusion.cpp
)

target_include_directories(run_tests PRIVATE /workspace/mocks/catch2)
target_link_libraries(run_tests PRIVATE catch2_mock core_lib comm_lib perception_lib render_lib pthread)

add_test(NAME run_tests COMMAND run_tests)
