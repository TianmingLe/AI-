add_library(catch2_mock STATIC /workspace/mocks/catch2/mock_main.cpp)
target_include_directories(catch2_mock PUBLIC /workspace/mocks/catch2)

add_executable(run_tests
    test_sanity.cpp
    test_config.cpp
    test_http.cpp
    test_http_concurrency.cpp
    test_udp.cpp
    test_gps.cpp
    test_sensor_fusion.cpp
    test_event_bus.cpp
    test_event_bus_backpressure.cpp
)

target_include_directories(run_tests PRIVATE /workspace/mocks/catch2)
target_link_libraries(run_tests PRIVATE catch2_mock core_lib comm_lib perception_lib render_lib pthread)
aigf_apply_target_options(run_tests)

add_test(NAME run_tests COMMAND run_tests)
