# /workspace/mocks/catch2/Catch2Config.cmake
set(Catch2_FOUND TRUE)
add_library(catch2_mock STATIC /workspace/mocks/catch2/mock_main.cpp)
target_include_directories(catch2_mock PUBLIC /workspace/mocks/catch2)
add_library(Catch2::Catch2WithMain ALIAS catch2_mock)
set(Catch2_INCLUDE_DIRS /workspace/mocks/catch2)
