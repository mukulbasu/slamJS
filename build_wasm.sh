emcc -std=c++17  -O3 -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=500MB -s WASM=1  \
    -DWASM_COMPILE=1 src/slamWasm.cpp  third_party/lib/g2o/js/libg2o.a src/imageAnalysis/orbExtractor.cpp third_party/lib/opencv/js/libopencv_flann.a third_party/lib/opencv/js/libopencv_calib3d.a third_party/lib/opencv/js/libopencv_core.a third_party/lib/opencv/js/libopencv_features2d.a third_party/lib/opencv/js/libopencv_objdetect.a third_party/lib/opencv/js/libopencv_imgproc.a third_party/lib/opencv/js/libopencv_photo.a \
     -Ithird_party/include/opencv4 -Ithird_party/include/ -Ithird_party/src/ -o slam.html

# emcc -std=c++17  -O2 -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=2GB -s WASM=1  \
#     -DWASM_COMPILE=1 src/slamWasm.cpp  third_party/src/g2o/**/*.cpp third_party/src/g2o/types/sba/*.cpp third_party/src/g2o/types/slam3d/*.cpp src/imageAnalysis/orbExtractor.cpp third_party/lib/opencv/js/libopencv_calib3d.a third_party/lib/opencv/js/libopencv_core.a third_party/lib/opencv/js/libopencv_features2d.a third_party/lib/opencv/js/libopencv_objdetect.a third_party/lib/opencv/js/libopencv_imgproc.a third_party/lib/opencv/js/libopencv_photo.a \
#      -Ithird_party/include/opencv4 -Ithird_party/include/ -Ithird_party/src/ -o slam.html

# emcc -std=c++17  -O2 -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=2GB -s WASM=1  \
#     -DWASM_COMPILE=1 src/slamWasm.cpp  third_party/src/g2o/**/*.cpp third_party/src/g2o/types/sba/*.cpp third_party/src/g2o/types/slam3d/*.cpp src/imageAnalysis/orbExtractor.cpp ../opencv/platforms/js/jspackage/lib/libopencv_core.a ../opencv/platforms/js/jspackage/lib/libopencv_features2d.a ../opencv/platforms/js/jspackage/lib/libopencv_objdetect.a ../opencv/platforms/js/jspackage/lib/libopencv_imgproc.a ../opencv/platforms/js/jspackage/lib/libopencv_photo.a \
#      -Ithird_party/include/opencv4 -Ithird_party/include/ -Ithird_party/src/ -o slam.html

rm slam.html
mv slam.js debug/js/slam.js
mv slam.wasm debug/js/slam.wasm
