cmake_minimum_required(VERSION 3.16)

project(ai_glasses_firmware LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(core_lib
    src/core/ConfigManager.cpp
    src/core/EventBus.cpp
    src/core/LogManager.cpp
)

add_library(comm_lib
    src/comm/BleClient.cpp
    src/comm/CoapClient.cpp
    src/comm/HttpClient.cpp
    src/comm/MqttClient.cpp
    src/comm/NtpClient.cpp
    src/comm/OpcUaClient.cpp
    src/comm/UdpBroadcaster.cpp
    src/comm/WebServer.cpp
)

add_library(perception_lib
    src/perception/AlsSensor.cpp
    src/perception/AudioInput.cpp
    src/perception/GpsSensor.cpp
    src/perception/ImuSensor.cpp
    src/perception/PowerSensor.cpp
    src/perception/SensorFusion.cpp
)

add_library(render_lib
    src/render/ArRenderer.cpp
)

target_link_libraries(comm_lib PRIVATE core_lib dl)
target_link_libraries(perception_lib PRIVATE core_lib)
target_link_libraries(render_lib PRIVATE core_lib)

add_executable(ai_glasses_firmware
    src/main.cpp
)

target_link_libraries(ai_glasses_firmware PRIVATE core_lib comm_lib perception_lib render_lib pthread)

enable_testing()
add_subdirectory(tests)
