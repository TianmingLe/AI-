# /workspace/mocks/catch_cmake/Catch.cmake
function(catch_discover_tests TARGET)
    add_test(NAME ${TARGET}_mock_test COMMAND ${TARGET})
endfunction()
