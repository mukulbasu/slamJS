cmake_minimum_required( VERSION 2.8 )
project( slamJS )

set(LINUX TRUE)
set(APPLE_M FALSE)
set(WIN FALSE)


set( CMAKE_BUILD_TYPE Debug )
set( CMAKE_CXX_FLAGS "-std=c++17 -Wall -O2 -march=native" )

set(G2O_HAVE_OPENGL 0)

include_directories( "third_party/include/" "third_party/include/suitesparse")
if(LINUX)
link_directories("../third_party/lib/opencv/x86_64-linux-gnu/" "../third_party/lib/g2o/x86_64-linux-gnu/" "../third_party/lib/suitesparse/x86_64-linux-gnu/")
elseif(APPLE_M)
link_directories("../third_party/lib/opencv/apple-m/" "../third_party/lib/g2o/apple-m/" "../third_party/lib/suitesparse/apple-m/")
endif()

add_compile_definitions(WASM_COMPILE=0)

file(GLOB library_sources 
    src/imageAnalysis/orbExtractor.cpp
)

add_library(${PROJECT_NAME}Library ${library_sources})

add_executable( ${PROJECT_NAME} src/slamTester.cpp )

if(LINUX)
target_link_libraries( ${PROJECT_NAME}
    opencv_calib3d opencv_flann opencv_imgcodecs libpng libjpeg-turbo libopenjp2 opencv_features2d
    opencv_imgproc opencv_core opencv_highgui 
    g2o cholmod amd colamd camd ccolamd suitesparseconfig
    zlib
    ${PROJECT_NAME}Library 
)
elseif(APPLE_M)
target_link_libraries( ${PROJECT_NAME} 
    tegra_hal opencv_imgcodecs libpng libjpeg-turbo libopenjp2 opencv_features2d 
    opencv_imgproc opencv_core opencv_highgui opencv_calib3d opencv_flann
    g2o cholmod amd colamd camd ccolamd suitesparseconfig
    zlib
    ${PROJECT_NAME}Library 
)
endif()