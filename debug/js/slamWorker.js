/**
 * @file slamWorker.js
 * @brief This contains the functionalities to work with slamWrapper.js for slam functionalities.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
let slam, imgBuffer, dataBuffer, float32DataBuffer;
let started = false, initialized = false;

// Setting up the Module (from slam.js), so that logs and status can be accessed
var Module = {
  preRun: [],
  postRun: [],
  print: (function() {
    return function(text) {
      if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
      console.log(text);
      postMessage({operation: "print", text});
    };
  })(),
  setStatus: function(text) {
    if (!Module.setStatus.last) Module.setStatus.last = { time: Date.now(), text: '' };
    if (text === Module.setStatus.last.text) return;
    var m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
    var now = Date.now();
    if (m && now - Module.setStatus.last.time < 30) return; // if this is a progress update, skip it if too soon
    Module.setStatus.last.time = now;
    Module.setStatus.last.text = text;
    if (m) {
      text = m[1];
      postMessage({operation: "status", complete: false, m, text});
    } else {
      postMessage({operation: "status", complete: true, text});
    }
  },
  totalDependencies: 0,
  monitorRunDependencies: function(left) {
    this.totalDependencies = Math.max(this.totalDependencies, left);
    Module.setStatus(left ? 'Preparing... (' + (this.totalDependencies-left) + '/' + this.totalDependencies + ')' : 'All downloads complete.');
  }
};
Module.setStatus('Downloading...');

importScripts("/js/slam.js"); 
importScripts("/js/slamConfigWasm.js"); //Contains the configs

Module.onRuntimeInitialized = async (_) => {
  postMessage({operation: "module_ready", from: "worker"});
};

//function to be invoked on completion of pose processing
const pose_processed = ({result, startTime, x, y, z, order}) => {
  const resultTranslation = (result) => {
    if (-6 == result) return "MATCH_INVALID";
    else if (-5 == result) return "NOT_ENOUGH_MATCH_FRAMES";
    else if (-4 == result) return "DID_NOT_MATCH_ALL_FRAMES";
    else if (-3 == result) return "NOT_ENOUGH_LANDMARKS";
    else if (-2 == result) return "ALREADY_INITIALIZED";
    else if (-1 == result) return "NOT_INITIALIZED";
    else if (0 == result) return "DEFAULT";
    else if (1 == result) return "VALID_MATCH";
    else return "UNKNOWN";
  }

  //Checking initialization
  if (!initialized && 0 != Module._is_initialized(slam)) {
    initialized = true;
    postMessage({operation: "initialized_map"});
  }
  console.log("Translations Added image ", Date.now() - startTime, "ms, Result is ", resultTranslation(result));

  const keyframeCount = Module._get_keyframe_count(slam);
  let translations, velocity;
  if (initialized) {
    translations = [Module._get_smooth_trans(slam, 0), Module._get_smooth_trans(slam, 1), Module._get_smooth_trans(slam, 2)];
    velocity = [Module._get_smooth_vel(slam, 0), Module._get_smooth_vel(slam, 1), Module._get_smooth_vel(slam, 2)];
  } else {
    translations = [0, 0, 0];
    velocity = [0, 0, 0];
  }
  postMessage({
    operation: "translations", 
    translations, 
    rotations: {x, y, z, order}, 
    velocity, 
    result: resultTranslation(result), 
    keyframeCount,
    processingTime: Date.now() - startTime
  });
}

//Setting up the function to handle incoming messages to worker
onmessage = (e) => {
  console.log("Message received from main script", e.data.operation);
  
  if (e.data.operation === "start") {
    //Initializing the WASM module and allocating the necessary memories.
    console.log("Initializing WASM Module");

    //Read and set the configs in SLAM Module
    const cfgReader = Module._config_reader();
    const encoder = new TextEncoder();
    console.log("MaxGap", SlamConfig["maxGap"]);
    for (const key in SlamConfig) {
      // console.log(key, SlamConfig[key]);
      const keyBytes = encoder.encode(key + '\0');
      const valBytes = encoder.encode(SlamConfig[key] + '\0');
      const keyPtr = Module._allocateMemory(keyBytes.length);
      const valPtr = Module._allocateMemory(valBytes.length);
      const keyMemory = new Uint8Array(wasmMemory.buffer, keyPtr, keyBytes.length);
      keyMemory.set(keyBytes);
      const valMemory = new Uint8Array(wasmMemory.buffer, valPtr, valBytes.length);
      valMemory.set(valBytes);
      Module._config_set(cfgReader, keyPtr, valPtr);
      Module._freeMemory(keyPtr);
      Module._freeMemory(valPtr);
    }
    slam = Module._initialize(cfgReader);

    console.log("Initialized SLAM: ", slam,". Now creating buffers");
    
    imgBuffer = Module._create_image_buffer(e.data.width, e.data.height);
    const dataBufferSize = Module._get_data_buffer_size();
    dataBuffer = Module._create_data_buffer();
    float32DataBuffer = new Float32Array(wasmMemory.buffer, dataBuffer, dataBufferSize);
    console.log("Created img buffer ", imgBuffer);
    console.log("Created data buffer ", dataBufferSize);
    started = true;
    initialized = false;

  } else if (e.data.operation === "process_image") {
    //Process image and extract pose
    const startTime = Date.now();
    if (!started) {
      console.error("process_image called without starting");
      setTimeout(() => postMessage({operation: "translations", translations: [0, 0, 0]}), 500);
      return;
    }
    const {imageData, x, y, z, order, timestamp} = e.data;
    Module.HEAP8.set(imageData.data, imgBuffer);
    console.log("Processing image");
    const result = Module._process_image(slam, imgBuffer, imageData.height, imageData.width, dataBuffer, x, y, z, timestamp); 
    console.log("Processing image done");
    pose_processed({result, startTime, x, y, z, order});

  } else if (e.data.operation === "process_keypoints") {
    //Process keypoints and extract pose
    const startTime = Date.now();
    if (!started) {
      console.error("process_keypoints called without starting");
      setTimeout(() => postMessage({operation: "translations", translations: [0, 0, 0]}), 500);
      return;
    }
    console.log("Processing keypoints");
    const {x, y, z, order, timestamp} = e.data;
    float32DataBuffer.set(e.data.data);
    const result = Module._process_keypoints(slam, dataBuffer, x, y, z, timestamp);
    console.log("Processing keypoints done");
    pose_processed({result, startTime, x, y, z, order});

  } else if (e.data.operation === "stop") {
    //Stop the SLAM module and free memory
    if (!started) {
      console.log("Already stopped");
      return;
    }
    console.log("Freeing buffer");
    Module._clear(imgBuffer);
    Module._clear(dataBuffer);
    console.log("Freed buffer. Freeing Slam");
    Module._clear(slam);
    console.log("Freed slam");
    started = false;
    initialized = false;

  } else if (e.data.operation === "keyframes") {
    //Fetch the list of keyframes
    let index = 0;
    translations = [];
    rotations = [];
    while (true) {
      const trans0 = Module._get_keyframe_trans(slam, 0, index); 
      if (trans0 === -9999) break;
      const trans1 = Module._get_keyframe_trans(slam, 1, index);
      const trans2 = Module._get_keyframe_trans(slam, 2, index);
      translations.push([ trans0, trans1, trans2 ]);

      const rot0 = Module._get_keyframe_rot(slam, 0, index); 
      const rot1 = Module._get_keyframe_rot(slam, 1, index);
      const rot2 = Module._get_keyframe_rot(slam, 2, index);
      rotations.push({x: rot0, y: rot1, z: rot2});
      index++;
    }
    postMessage({operation: "keyframes", translations, rotations});

  }
};