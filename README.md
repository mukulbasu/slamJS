#What is SlamJS
SlamJS is a free, opensource, easy-to-use and understand JavaScript library for camera pose estimation for VR, AR, XR use on mobile browsers. It is not yet in a production-ready state. However, the hope is that it provides a starting point for anyone wanting to experiment, play around with SLAM and its concepts.

#Demo Video

https://github.com/mukulbasu/slamJS/assets/1706979/688b67ba-524e-4b4d-9edb-1957472d033f


#Installation:
1. Make sure gcc, cmake, yarn are installed.
1. To build for Web, you have to set up emscripten. Follow the instructions from https://emscripten.org/docs/getting_started/downloads.html

#Build and Running:
The application can be built and run locally against a test dataset or it can be run on the web browser. A sample dataset is provided in data/dataTest. The user can also choose to generate their own dataset for tests using the tools provided. 

##Building and running on Local machine:
This has been tested primarily on Ubuntu 22.04 LTS and Mac M2. 

NOTE: Before building on Mac, open CMakeLists.txt and change `set(LINUX TRUE)` to `set(LINUX FALSE)`. Also change `set(APPLE_M FALSE)` to `set(APPLE_M TRUE)`.

###Building for local machine:
1. Go to top level directory
1. `yarn install`
1. `mkdir build; cd build`
1. `cmake ..`
1. `make -j10`
Any time you update the libraries in the third_party folders, remember to `rm -rf *` everything in build folder and then redo from step 3 (cmake and so on).

Also the libraries to be copied to third_party/lib are all opencv and can be generated from https://github.com/mukulbasu/opencv.
Follow the cpp build process mentioned in the README and copy over the libraries from the build/lib/* and build/3rdparty/lib/* into the third_party/lib/opencv/<arch> folder.


###Running on local machine:
The executable binary is created in build directory as *slamJS*. A sample configuration file *config/slamConfigMobile.js* is provided. A test dataset *data/dataTest* is also present.
1. Go to top level directory
1. `build/slamJS slamConfigMobile > debug/tmp`
1. After this command completes running, move to Debugging.

###Debugging:
The main debug website is built on React. The actual debug data on the website is served by a separate Node server *(debug/server.js)* that reads and serves data from *debug/logs/debug.txt* and *debug/tmp*. These files are generated once the command above is run.
1. Go to top level directory
1. `cd debug`
1. `npm install; source startServer.sh`
1. Open another terminal and in that type `cd debug/react; npm install; npm start`.
1. Open http://localhost:3000. Follow instructions on screen.


##Building and running on Web:
This has been tested only on Android so far.

###Building for Web:
1. Activate emscripten environment.
1. in the top level directory run: source build_wasm.sh

###Running on Web:
Set up ADB wireless connection between your Android device and the local machine `https://developer.android.com/tools/adb`. 
1. Go to top level directory
1. cd debug
1. source startServer.sh
1. Set up your android device for ADB connection. 
1. On your local machine, open Google Chrome and point it to `chrome://inspect/#devices`. Make sure the port forwarding settings are set up to forward 8080 to localhost:8080.
1. On your Android Chrome browser open `http://localhost:8080/slamDemo.html`
1. Click `Start AR` to start the AR operation.
1. `Stop AR` is not tested yet. Reload the page instead to retest.

##Building third party libraries
Pre-built binaries for third_party libraries are provided for linux and Mac ARM architecture. For other architectures, you might have build the binaries yourself as follows:

There are 2 main libraries in the folder : 1) Libraries from OpenCV  2) Libraries from Suitesparse

1. OpenCV: The libraries to be copied to third_party/lib are all opencv and can be generated from https://github.com/mukulbasu/opencv, branch: slamjs.
Follow the cpp build process mentioned in the README and copy over the libraries from the build/lib/* and build/3rdparty/lib/* into the third_party/lib/opencv/<arch> folder.
1. Suitesparse: Clone https://github.com/DrTimothyAldenDavis/SuiteSparse. 
    a. You might need to install openBLAS on linux using - sudo apt-get install libopenblas-dev
    b. In the top level directory, run the command: CMAKE_OPTIONS="-DBLA_VENDOR=OpenBLAS" make -j20
    c. Go to build folder inside each of the respective directories like - CHOLMOD, AMD etc and copy the library from there to third_party/lib/suitesparse/<arch>/
1. G2O: The source code for the same is included in the third_party/src directory. Follow the instructions in README.md in third_party/src/go/ to build it for the required platform.

##Running locally with your test data
Test data can be generated this way:

1. Go to top level directory
1. `mkdir data`
1. `cd debug`
1. `source startServer.sh`
1. On your mobile browser open - *http://localhost:8080/test.html*
