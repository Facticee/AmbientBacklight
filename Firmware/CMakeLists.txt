cmake_minimum_required(VERSION 3.21)
project(Ambilight LANGUAGES CXX)

add_executable(Ambilight main.cpp)
target_compile_features(Ambilight PRIVATE cxx_std_17)
target_compile_definitions(Ambilight PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(Ambilight PRIVATE d3d11 dxgi d3dcompiler)

add_custom_command(TARGET Ambilight POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/shader.hlsl" "$<TARGET_FILE_DIR:Ambilight>/shader.hlsl")
