cmake_minimum_required(VERSION 3.16)

project(ai_glasses_firmware LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

option(AIGF_ENABLE_WARNINGS "Enable compiler warnings" ON)
option(AIGF_ENABLE_WERROR "Treat warnings as errors" OFF)
option(AIGF_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(AIGF_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(AIGF_ENABLE_TSAN "Enable ThreadSanitizer" OFF)

if(AIGF_ENABLE_TSAN AND (AIGF_ENABLE_ASAN OR AIGF_ENABLE_UBSAN))
    message(FATAL_ERROR "TSAN cannot be combined with ASAN/UBSAN")
endif()

set(AIGF_SAN_FLAGS "")
if(AIGF_ENABLE_ASAN)
    list(APPEND AIGF_SAN_FLAGS "address")
endif()
if(AIGF_ENABLE_UBSAN)
    list(APPEND AIGF_SAN_FLAGS "undefined")
endif()
if(AIGF_ENABLE_TSAN)
    list(APPEND AIGF_SAN_FLAGS "thread")
endif()

function(aigf_apply_target_options tgt)
    if(AIGF_ENABLE_WARNINGS AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
        target_compile_options(${tgt} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wshadow
            -Wnon-virtual-dtor
            -Woverloaded-virtual
            -Wnull-dereference
            -Wdouble-promotion
            $<$<BOOL:${AIGF_ENABLE_WERROR}>:-Werror>
        )
        target_compile_options(${tgt} PRIVATE
            -fstack-protector-strong
            -D_FORTIFY_SOURCE=2
        )
        target_link_options(${tgt} PRIVATE
            -Wl,-z,relro
            -Wl,-z,now
        )
    endif()

    if(AIGF_SAN_FLAGS)
        list(JOIN AIGF_SAN_FLAGS "," AIGF_SAN_FLAGS_JOINED)
        target_compile_options(${tgt} PRIVATE -fsanitize=${AIGF_SAN_FLAGS_JOINED} -fno-omit-frame-pointer)
        target_link_options(${tgt} PRIVATE -fsanitize=${AIGF_SAN_FLAGS_JOINED})
    endif()
endfunction()

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

aigf_apply_target_options(core_lib)
aigf_apply_target_options(comm_lib)
aigf_apply_target_options(perception_lib)
aigf_apply_target_options(render_lib)

target_link_libraries(comm_lib PRIVATE core_lib dl)
target_link_libraries(perception_lib PRIVATE core_lib)
target_link_libraries(render_lib PRIVATE core_lib)

add_executable(ai_glasses_firmware
    src/main.cpp
)

target_link_libraries(ai_glasses_firmware PRIVATE core_lib comm_lib perception_lib render_lib pthread)
aigf_apply_target_options(ai_glasses_firmware)

enable_testing()
add_subdirectory(tests)
