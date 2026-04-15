# /workspace/mocks/opencv/OpenCVConfig.cmake
set(OpenCV_FOUND TRUE)
set(OpenCV_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include)

add_library(opencv_core INTERFACE)
target_include_directories(opencv_core INTERFACE ${OpenCV_INCLUDE_DIRS})

add_library(opencv_highgui INTERFACE)
target_include_directories(opencv_highgui INTERFACE ${OpenCV_INCLUDE_DIRS})

set(OpenCV_LIBS opencv_core opencv_highgui)
