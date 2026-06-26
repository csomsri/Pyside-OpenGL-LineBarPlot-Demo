cmake_minimum_required(VERSION 3.10)

project(GraphOpenGl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

add_executable(GraphOpenGl WIN32 main.cpp)
target_link_libraries(GraphOpenGl PRIVATE OpenGL::GL)

find_package(Python COMPONENTS Interpreter Development.Module QUIET)

if(Python_FOUND)
    set(PYTHON_MODULE_LIBRARIES "")
    if(_Python_LIBRARY_RELEASE)
        set(PYTHON_MODULE_LIBRARIES "${_Python_LIBRARY_RELEASE}")
    else()
        set(PYTHON_MODULE_LIBRARIES ${Python_LIBRARIES})
    endif()

    add_library(native_graph MODULE native_graph.cpp)
    target_include_directories(native_graph PRIVATE ${Python_INCLUDE_DIRS})
    target_link_libraries(native_graph PRIVATE OpenGL::GL ${PYTHON_MODULE_LIBRARIES})

    set(PYTHON_MODULE_SUFFIX ".pyd")
    if(Python_SOABI)
        set(PYTHON_MODULE_SUFFIX ".${Python_SOABI}.pyd")
    endif()
    set_target_properties(native_graph PROPERTIES
        PREFIX ""
        SUFFIX "${PYTHON_MODULE_SUFFIX}"
    )
else()
    message(STATUS "Python development files were not found; skipping native_graph Python module.")
endif()
