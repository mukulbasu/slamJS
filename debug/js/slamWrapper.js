/**
 * @file slamWrapper.js
 * @brief This contains the functionalities for running SLAM with slamDemo.html.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */

import {getRotation, trim, arRender, debugPoseRender} from '/js/utils.js';

const srcVideo = document.querySelector("#srcVideo");
const newWidth = Math.round(srcVideo.offsetHeight * width / height);
const origWidth = srcVideo.offsetWidth;
const dimStyle = `height: ${srcVideo.offsetHeight}px; width: ${newWidth}px; margin-left: -${(newWidth-origWidth)/2}px`;
console.log("Dimensions", dimStyle, srcVideo.offsetHeight, srcVideo.offsetWidth);
srcVideo.setAttribute("style", dimStyle);
document.getElementById("threejs-ar-container").setAttribute("style", `height: ${srcVideo.offsetHeight};width: ${origWidth}`);

const canvas = document.querySelector("#canvasVideo");
canvas.setAttribute("style", dimStyle);
canvas.width = width;
canvas.height = height;
const ctx = canvas.getContext("2d", { willReadFrequently: true });

let cameraAR = null;
let rotation = "";
let acceleration = {x : 0, y : 0, z : 0};
let latestTrans = [0, 0, 0];
let latestVel = [0, 0, 0];
let startAR = false;
let frameProcessTime = Date.now();
let kpExtractTime = Date.now();
let debugPose;

const handleOrientation = (event) => {
  rotation = getRotation(event);
  if (!startAR && cameraAR != null) {
    console.log("Rotation noAR ", trim(rotation.x*180/Math.PI), trim(rotation.y*180/Math.PI), trim(rotation.z*180/Math.PI));
    cameraAR.update([0, 0, 0], rotation);
    debugPose.updateCurrentFrame([0, 0, 0], rotation);
  }
}

let lastVelComputed = [0, 0, 0];
let lastTransComputed = [0, 0, 0];
let lastComputeTimestamp = -1;
const handleAcceleration = (event) => {
  const {x, y, z} = event.acceleration;
  acceleration = [x, y, z];
  if (-1 == lastComputeTimestamp) {
    lastComputeTimestamp = Date.now();
    lastVelComputed = latestVel;
    lastTransComputed = latestTrans;
    return;
  }
  const newTime = Date.now();
  const timeDiff = (newTime - lastComputeTimestamp)/1000;
  const newVel = [0, 0, 0];
  for (let i = 0; i < 3; i++) {
    newVel[i] = lastVelComputed[i] + acceleration[i]*timeDiff;
    lastTransComputed[i] = (newVel[i] + lastVelComputed[i])/2*timeDiff;
  }
  lastVelComputed = newVel;
}

//Variables for SLAM
let slam, imgBuffer, dataBuffer, float32DataBuffer, transferBuffer;
let started = false, modulesReady = new Set();

//Initializes the SLAM module
const startSlamModule = ({width, height}) => {
  console.log("Initializing Slam Module");
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
  console.log("Initialized SLAM: ", slam,". Creating buffer");
  imgBuffer = Module._create_image_buffer(width, height);
  const dataBufferSize = Module._get_data_buffer_size();
  dataBuffer = Module._create_data_buffer();
  float32DataBuffer = new Float32Array(wasmMemory.buffer, dataBuffer, dataBufferSize);
  transferBuffer = new Float32Array(dataBufferSize);
  // dataBuffer = new Float32Array(sizeDataBuffer*100);
  console.log("Created img buffer ", imgBuffer);
  console.log("Created data buffer ", dataBufferSize);
  started = true;
}

//Toggles visibility of stats and 3D Pose Viewer
let toggleDebug = true;
document.getElementById("toggleDebug").onclick = () => {
  if (toggleDebug) {
    toggleDebug = false;
    document.getElementById("stats").style.visibility = "hidden";
    document.getElementById("threejs-debug-pose-container").style.visibility = "hidden";
  } else {
    toggleDebug = true;
    document.getElementById("stats").style.visibility = "visible";
    document.getElementById("threejs-debug-pose-container").style.visibility = "visible";
  }
}

//Handle AR Start/Stop
document.getElementById("startAR").onclick = () => {
  const dom = document.getElementById("startAR")
  if (startAR) {
    startAR = false;
    dom.innerText = "Start AR";
    slamWorker.postMessage({operation: "stop", width, height});
    if (SlamConfig.processKeyPointsInMainThread) {
      console.log("Freeing buffer");
      Module._clear(imgBuffer);
      Module._clear(dataBuffer);
      console.log("Freed buffer. Freeing Slam");
      Module._clear(slam);
      console.log("Freed slam");
      started = false;
    }
    document.getElementById("status").innerText = "AR Stopped";
  } else {
    slamWorker.postMessage({operation: "start", width, height});
    if (SlamConfig.processKeyPointsInMainThread) startSlamModule({width, height});
    startAR = true;
    dom.innerText = "Stop AR";
  }
}

//Process the captured image
const processImage = ({imageData, x, y, z, order}) => {
  if (SlamConfig.processKeyPointsInMainThread ) {
    //Extract keypoints in the main thread and send it to worker thread
    if (!started) {
      console.error("Add Image called without starting");
      setTimeout(() => eventHandler({data:{operation: "translations", translations: [0, 0, 0]}}), 500);
      return;
    }
    Module.HEAP8.set(imageData.data, imgBuffer);

    console.log("Extracting KPs");
    const extractStartTime = Date.now();
    Module._extract_keypoints(slam, imgBuffer, imageData.height, imageData.width, dataBuffer);
        
    transferBuffer.set(float32DataBuffer);
    //Send keypoints to worker for pose detection
    slamWorker.postMessage({operation: "process_keypoints", data: transferBuffer, x, y, z, order, timestamp: extractStartTime});
    kpExtractTime = Date.now() - extractStartTime;
  } else {
    //Send the image to worker. Both the keypoint extraction and pose detection will happen in worker
    slamWorker.postMessage({operation: "process_image", imageData, x, y, z, order, timestamp: Date.now()});
    kpExtractTime = 0;
  }

  frameProcessTime = Date.now();
}

//Capture image for processing pose
let queueCount = 0;
const captureImage = () => {
  ctx.drawImage(srcVideo, 0, 0, width, height);
  if (!startAR) return;
  const imageData = ctx.getImageData(0, 0, width, height);
  setTimeout(captureImage, 0);

  if (queueCount < 2) {
    console.log("Capturing image");
    processImage({...rotation, imageData});
    queueCount = queueCount + 1;
  }
};

//Start the camera
const startVideo = (mediaStream) => {
  console.log("Got the web cam stream");
  srcVideo.srcObject = mediaStream;
  
  const frameRate = mediaStream.getVideoTracks()[0].getSettings().frameRate;
  setInterval(captureImage, 1000/frameRate);

  window.addEventListener("deviceorientation", handleOrientation);
  window.addEventListener("devicemotion", handleAcceleration);
  document.getElementById("startAR").disabled = false;
  cameraAR = arRender("threejs-ar-container", 180.0*Math.atan(640.0/(250*2))/Math.PI);
  debugPose = debugPoseRender("threejs-debug-pose-container");
};

const slamWorker = new Worker("js/slamWorker.js");
let camX = 0, camY = 0, camZ = 0;
let lastTransTime = Date.now();

//Handle incoming messages from Worker
const eventHandler = (e) => {
  if (e.data.operation === "translations") {
    console.log("Translations");
    const trans = e.data.translations;
    const rot = e.data.rotations;
    const ratios = [];
    ratios[0] = (trans[0] - latestTrans[0])/(lastTransComputed[0] - latestTrans[0]);
    ratios[1] = (trans[1] - latestTrans[1])/(lastTransComputed[1] - latestTrans[1]);
    ratios[2] = (trans[2] - latestTrans[2])/(lastTransComputed[2] - latestTrans[2]);

    console.log("Translations: ", trans[0], trans[1], trans[2], ", ", Date.now() - frameProcessTime, "ms",
      "lastTransComputed", lastTransComputed[0]-latestTrans[0], "vs", trans[0] - latestTrans[0], 
      ":", lastTransComputed[1]-latestTrans[1], "vs", trans[1] - latestTrans[1], 
      "ratios", ratios[0], ratios[1], ratios[2]);

    latestTrans = [...e.data.translations];
    latestVel = [...e.data.velocity];
    lastVelComputed = [...e.data.velocity];
    lastTransComputed = [...e.data.translations];
    document.getElementById("pose").textContent = `Pose : ${trim(trans[0])}, ${trim(trans[1])}, ${trim(trans[2])}`;
    const weight = 1.0;
    console.log("Result ", e.data.result);
    const statusDom = document.getElementById("status");
    if (statusDom.innerText != "Initialized") {
      statusDom.innerText = `Initializing (KeyFrames - ${e.data.keyframeCount})`;
    }
    if (null != cameraAR) {
      console.log("Rotation AR ", trim(rot.x*180/Math.PI), trim(rot.y*180/Math.PI), trim(rot.z*180/Math.PI));
      cameraAR.update(latestTrans, rot);
    }
    debugPose.updateCurrentFrame(latestTrans, rot);
    document.getElementById("time").textContent = `Time : Last Trans = ${(Date.now() - lastTransTime).toString()} / Frame Process = ${(Date.now() - frameProcessTime).toString()} / KP Extract = ${kpExtractTime} ms`;
    document.getElementById("result").textContent = `Result : ${e.data.result}`;
    
    queueCount = queueCount - 1;
    lastTransTime = Date.now();

  } else if(e.data.operation == "initialized_map") {
    //SLAM Map has been initialized
    document.getElementById("status").innerText = "Initialized";

  } else if (e.data.operation === "print") {
    // const matchVsId = { "Found Focus": "focus", "Time KeyFrame": "perf", "Matching with": "matches"};
    const matchVsId = { "Selected Focus": "focus", "KP Time": "perf", "Time Profile": "perf2"};
    for (const match in matchVsId) {
      if (e.data.text.includes(match)) {
        document.getElementById(matchVsId[match]).textContent = e.data.text;
        break;
      }
    }

  } else if (e.data.operation === "status") {
    const text = e.data.text;
    if (e.data.complete == false) {
      console.log("Download complete");
    }


  } else if (e.data.operation === "keyframes") {
    //Update the list of keyframes in the 3d Viewer for debugging
    const keyframeTrans = e.data.translations;
    const keyframeRots = e.data.rotations;
    debugPose.updateKeyframes(keyframeTrans, keyframeRots);

  } else if (e.data.operation === "module_ready") {
    //Indicates WASM module has been downloaded and is ready.
    //If main thread is going to do keypoint processing, then check that
    //both worker and main thread are ready.
    console.log("MODULE READY", e.data.from);
    modulesReady.add(e.data.from);
    if (SlamConfig.processKeyPointsInMainThread) {
      if (!modulesReady.has("worker") || !modulesReady.has("wrapper")) return;
    } else {
      if (!modulesReady.has("worker")) return;
    }
    navigator.mediaDevices.getUserMedia(
      {audio: false, video: {facingMode:{exact: "environment"}, frameRate: { ideal: 10, max: 10 }}}
    ).then(startVideo)
    .catch((error) => {
      console.error("ERROR, could not start back facing camera. Trying front facing");
      console.error(error);
      navigator.mediaDevices.getUserMedia({ video: true }).then(startVideo);
    });
    setInterval(() => {
      if (document.getElementById("status").innerHTML == "Initialized")
        slamWorker.postMessage({operation: "keyframes"});
    }, 2000);
  }
}

slamWorker.onmessage = eventHandler;

if (SlamConfig.processKeyPointsInMainThread) {
  // Setting up the Module (from slam.js), so that logs and status can be accessed
  // This is necessary only if the keypoints will be extracted in main thread
  Module.onRuntimeInitialized = async function() {
    console.log("Module runtime is iniitialized");
    eventHandler({data:{operation: "module_ready", from: "wrapper"}});
  };
  if (typeof Module._config_reader === 'function') {
    //Already module has been loaded.
    Module.onRuntimeInitialized();
  }
  Module.print = async function(text) {
    if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
    // console.log(text);
    eventHandler({data: {operation: "print", text}});
  };
  console.log("Configured onRuntime MODULE READY");
}