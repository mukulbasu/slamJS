Instructions for building g2o.

For apple-m or x86_64 linux
1. mkdir build
2. cd build
3. rm -rf *
4. cmake ..
5. make -j10
6. cp libg2o.a ../../../lib/g2o/<arch>/


For JS WASM build
It is recommended to build this on an Ubuntu machine. Might not work on Mac.
1. Activate emscripten environment. (By going to emsdk directory and running 'source emsdk_env.sh')
2. Come back to slamJS/third_party/src/g2o
1. mkdir build
2. cd build
3. rm -rf *
4. emcmake cmake ..
5. make -j10
6. cp libg2o.a ../../../lib/g2o/js/