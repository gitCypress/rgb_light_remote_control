# 针对目标启用并设置C++标准
enable_language(CXX)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_EXTENSIONS OFF)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/Core/Src/maincxx.cpp
        ${CMAKE_SOURCE_DIR}/Core/Src/retarget.c
        ${CMAKE_SOURCE_DIR}/Core/Src/app/driver/ws2812b.cpp
        ${CMAKE_SOURCE_DIR}/Core/Src/app/driver/esp8266.cpp
        ${CMAKE_SOURCE_DIR}/Core/Src/app/uart_receiver.cpp
        ${CMAKE_SOURCE_DIR}/Core/Src/app/ProtocolHandler.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/Core/Inc/app
        ${CMAKE_SOURCE_DIR}/Core/Inc/app/driver
)
