cmake_minimum_required(VERSION 3.8)

set(EXE ray)
set(SRC_DIR src)

add_subdirectory(ext/SDL)
add_subdirectory(ext/GLM)

add_executable(${EXE} ${SRC_DIR}/main.cpp ext/gl3w/src/gl3w.c "src/shader.hpp")

set_property(TARGET ${EXE} PROPERTY CXX_STANDARD 20)
target_include_directories(${EXE} PRIVATE ${SRC_DIR}/ ext/gl3w/include/ ext/stb/)
target_link_libraries(${EXE} PRIVATE SDL2::SDL2-static glm::glm ${CMAKE_DL_LIBS})

file(COPY res/ DESTINATION res/)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/drawTriangles.vert ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/drawTriangles.frag ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/genTriangles.comp ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/dynTriangles.comp ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
