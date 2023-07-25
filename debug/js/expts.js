/**
 * @file slamConfigModule.js
 * @brief This is a test file ignore.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */

const trim = (value) => Math.round(value*100)/100;

let alpha, beta, gamma, acc = 0, lastAcc = 0, totalAcc = 0, lastValue = 0;
function handleOrientation(event) {
  ({alpha, beta, gamma} = event);
}

let state = 0, count = 0, rots = 0, xInit = 0, yInit = 0, zInit = 0;
let startTime, endTime, rotTime, rotDuration = 0;
let smoothX = 0, lastSmoothX = 0, newRot = 0;

const getAccDirection = (event) => {
    const ea = event.acceleration;
    const x = ea.x > 0.3? 1: ea.x < -0.3? -1: 0;
    const y = ea.y > 0.3? 1: ea.y < -0.3? -1: 0;
    const z = ea.z > 0.3? 1: ea.z < -0.3? -1: 0;
    return [x, y, z];
}

const ifOpp = (val1, val2) => {
    if (val1 == -val2) return true;
    return false;
}

const ifOpps = (x, y, z, xInit, yInit, zInit) => {
    let count = 0;
    if (ifOpp(x, xInit)) count = count+1;
    if (ifOpp(y, yInit)) count = count+1;
    if (ifOpp(z, zInit)) count = count+1;
    return count > 1;
}


// 1 -1 1 -1 = 1 => 3
// 1 -1 1 -1 1 -1 = 2 => 5
// 1 -1 1 -1 1 -1 1 -1 = 3 => 7
let printCnt = 0
let scale = -1;
let oldX = -1, xAccCount = 0, xDist = 0, lastXTime = Date.now();
let distX = 0, velX = 0, countX = 0, totalDistX = 0;
let lastAccX = -1, accXs = [];
function handleAcceleration(event) {
    printCnt++;

    if (event.acceleration.x * lastAccX < 0) {
      let length = Math.round((accXs.length + 1)/2);
      length = length < 0? 0 : length;
      const accXs1 = accXs.slice(0, length);
      accXs1.forEach( acc => {
        velX += acc;
        distX += Math.abs(velX);
      });
      // const avgDistX = totalDistX == 0? 0 : totalDistX / countX;
      const avgDistX = totalDistX;
      console.log("Dist total ", distX, ", ", avgDistX, ", ", accXs.length);
      const minX = 0.6 * avgDistX;
      const maxX = 1.4 * avgDistX;
      if (distX > 500) {
        if (0 == totalDistX) {
          totalDistX = distX;
          countX = 1;
        // } else if (distX > minX && distX < maxX) {
        } else {
          totalDistX = totalDistX*0.8 + 0.2*distX;
          countX += 1;
        }
      }
      distX = 0;
      velX = 0;
      const accXs2 = accXs.slice(length);
      accXs2.forEach(acc => {
        velX += acc;
        distX += Math.abs(velX);
      });
      accXs = []
    }
    accXs.push(event.acceleration.x);

    if (event.acceleration.x != 0) lastAccX = event.acceleration.x;

    console.log("Values :", trim(event.acceleration.x), ", ", alpha);
    // console.log("Acceleration Values :", trim(event.acceleration.x), ", ", trim(event.acceleration.y),  ", ", trim(event.acceleration.z));
    const value = Math.sqrt(Math.pow(event.acceleration.x, 2) 
            + Math.pow(event.acceleration.y, 2) + Math.pow(event.acceleration.z, 2));
            

    if (event.acceleration.x >= 0 && oldX <= 0 || event.acceleration.x <= 0 && oldX >= 0) {
      // console.log("Acc count ", xAccCount, " distance ", xDist);
      xAccCount = 0;
      xDist = 0;
    } else {
      xAccCount += 1;
      xDist += (oldX+event.acceleration.x) * Math.pow((Date.now() - lastXTime), 3) / (2*1000);
    }
    oldX = event.acceleration.x;
    lastXTime = Date.now();
    lastAcc = acc;
    acc = (3*acc + 2*value)/5;
    if (state == 1 && count < 1) {
        count = count + 1;
    } else if (state == 1 && count == 1) {
        state = 2;
    } 
    smoothX = (smoothX * 4 + event.acceleration.x) / 5;
    if (state == 2) {
        const tempSmoothX = smoothX > 0.2? 1: smoothX < -0.2? -1: 0;
        // if ((smoothX > 0 && lastSmoothX < -0) || (smoothX < -0 && lastSmoothX > 0)) {
        if (tempSmoothX != lastSmoothX && tempSmoothX != 0) {
            newRot = newRot + 1;
            // console.log("New rot val: ", newRot, lastSmoothX, smoothX);
            lastSmoothX = tempSmoothX;
        }
    }
    
    if (state == 2 && (acc < 1 || value < lastValue*0.3 || value > lastValue*3)) {
        state = 0;
        // console.log("Acc ", value);
        const tempRotDuration = Date.now() - rotTime;
        if ((rotDuration == 0 && tempRotDuration > 200) || (rotDuration != 0 && tempRotDuration > rotDuration * 0.3)) {
            const [x, y, z] = getAccDirection(event);
            console.log("Rots: ", x, y, z, xInit, yInit, zInit, (rots)/2, rotDuration, tempRotDuration);
            if (ifOpps(x, y, z, xInit, yInit, zInit)) {
                rots = rots + 1;
                [xInit, yInit, zInit] = [x, y, z];
                rotTime = Date.now();
                rotDuration = tempRotDuration>rotDuration?tempRotDuration:rotDuration;
            }
        }
        endTime = Date.now();
        const duration = endTime - startTime;
        if (duration > 2000) {
            console.log("Stopped");
            console.log("Total: ", totalAcc);
            const rotation = (newRot - 2)/2;//(rots)/2;
            console.log("Total Rots: ", rotation);
            console.log("Total Distance: ", totalAcc * (duration/1000)/(15*rotation*rotation));
            scale = totalAcc * (duration/1000)/(15*rotation*rotation);
            console.log("New rot: ", newRot, (newRot - 2)/2);
        } else {
            console.log("Ignored");
            console.log("Rots ignored");
        }
    } else if (state == 2) {
        // console.log("Acc ", value);
        totalAcc = totalAcc + value;
        const tempRotDuration = Date.now() - rotTime;
        if ((rotDuration == 0 && tempRotDuration > 200) || (rotDuration != 0 && tempRotDuration > rotDuration * 0.3)) {
            const [x, y, z] = getAccDirection(event);
            // console.log("Rots: ", x, y, z, xInit, yInit, zInit, (rots)/2, rotDuration, tempRotDuration);
            if (ifOpps(x, y, z, xInit, yInit, zInit)) {
                rots = rots + 1;
                [xInit, yInit, zInit] = [x, y, z];
                rotTime = Date.now();
                rotDuration = tempRotDuration>rotDuration?tempRotDuration:rotDuration;
            }
        }
    } else if (acc > 3 && state == 0) {
        // console.log("Acc ", value);
        state = 1;
        count = 0;
        rots = 0;
        [xInit, yInit, zInit] = getAccDirection(event);
        rotTime = Date.now();
        rotDuration = 0;
        // console.log("Rots Init: ", xInit, yInit, zInit, rots);
        startTime = Date.now();
        totalAcc = 0;
        newRot = 0;
        console.log("Start Timer");
    }
    lastValue = value;
}



const startVideo = (mediaStream) => {
  console.log("Got the web cam stream");
  const srcVideo = document.querySelector("#srcVideo");
  srcVideo.srcObject = mediaStream;
  const canvas = document.querySelector("#canvasVideo");
  const ctx = canvas.getContext("2d", { willReadFrequently: true });

  console.log("Initializing");
  const slam = Module._initialize();
  console.log("Initialized SLAM: ", slam,". Creating buffer");
  const buffer = Module._create_image_buffer(srcVideo.width, srcVideo.height);
  console.log("Created buffer ", buffer);

  setInterval(() => {
    // ctx.drawImage(srcVideo, 0, 0, canvas.width, canvas.height);
  }, 200);

  const compute = () => {
    ctx.drawImage(srcVideo, 0, 0, canvas.width, canvas.height);
    const imageData = ctx.getImageData(0, 0, srcVideo.width, srcVideo.height);
    Module.HEAP8.set(imageData.data, buffer);
    // console.log("Copied");
    console.log("Adding image");
    const result = Module._add_image(slam, buffer, imageData.height, imageData.width, alpha, beta, gamma, 10, 200);
    console.log("Added image");
    console.log("Translations: ", Module._get_trans(slam, 0), Module._get_trans(slam, 1), Module._get_trans(slam, 2), "\n\n\n");
    setTimeout(compute, 200);
  };

  // setTimeout(compute, 2000);

  document.querySelector("#stop").onclick = () => {
    clearInterval(interval);
    setTimeout(() => {
      console.log("Freeing buffer");
      Module._clear(buffer);
      console.log("Freed buffer. Freeing Slam");
      Module._clear(slam);
      console.log("Freed slam");
    }, 1000);
  };

  window.addEventListener("deviceorientation", handleOrientation);
  window.addEventListener("devicemotion", handleAcceleration);
};

Module.onRuntimeInitialized = async (_) => {

  // navigator.mediaDevices.getUserMedia({ video: true }).then((mediaStream) => {
  navigator.mediaDevices.getUserMedia(
    {audio: false, video: {facingMode:{exact: "environment"}, frameRate: { ideal: 10, max: 15 }}}
  ).then(startVideo)
  .catch((error) => {
    console.error("ERROR, could not start back facing camera. Trying front facing");
    navigator.mediaDevices.getUserMedia({ video: true }).then(startVideo);
  });
};