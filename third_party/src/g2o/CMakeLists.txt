cmake_minimum_required( VERSION 2.8 )
project( g2o )

set( CMAKE_BUILD_TYPE Release )
set( CMAKE_CXX_FLAGS "-std=c++17 -Wall -O2 -march=native" )

set(G2O_HAVE_OPENGL 0)

include_directories( "../../include/" "../../include/suitesparse")

file(GLOB library_sources 
    **/*.cpp 
    types/sba/*.cpp 
    types/slam3d/*.cpp
)

add_library(${PROJECT_NAME} ${library_sources})
