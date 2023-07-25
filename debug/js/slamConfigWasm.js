/**
 * @file slamConfigWasm.js
 * @brief This contains the configurations for running and debugging SLAM on web.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
const pathStart = 0;
const pathEnd = 10;
const dataFolder = "dataTest";
const fileExtension = "png";

const width = 480*3/4;
const height = 640*3/4;
const ratio = (width / 480);
console.log("ratio ", ratio);
const focus = 350 * ratio;//200 also works good


const SlamConfig = {
    cx: (240*ratio).toString(),
    cy: (317*ratio).toString(),
    fx: focus.toString(),
    fy: focus.toString(),
    maxDepth: "75",

    matchHierarchy: "t",
    leafSize: "27",
    branchSize: "3",
    treeSize: "3",

    reqdKpsInit: "1000",
    reqdKps: "500",

    maxGap: (100 * ratio).toString(), //300
    minGap: "2",
    minAvgGapInit: "10",
    minAvgGap: "0",
    distanceThreshold: "64",
    ratio: "0.7",
    imgWidthRatio: ratio.toString(),
    debugEstimateValidation: "f",

    pathStart: pathStart.toString(),
    pathEnd: pathEnd.toString(),//190",
    scale: "0.5",

    baOption: "1",
    maxFrames: "20",
    mapInitializationFrames: "2",
    numKeyFrameMatches: "4",//3
    maxDistRatio: "0.2",
    maxAngle: "30",
    copyRotation: "t",
    findFocus: "f",
    normalizeKP: "t",
    debugFrameId: "-1",
    disableRotationInput: "f",
    newKeyframesBA: "f",
    smootheningTolerance: "0.02", //This makes the position stick to previous values unless sufficient movement is noticed
    cholmod: "t", //Set to f for using Eigen. This seems to optimize time without any impact on accuracy.

    //May be need to be deleted

    paths: [
        "0", "-14", "10",
    ],
    fileExtension,
    path: "data/" + dataFolder + "/image",
    orient: "data/" + dataFolder + "/orient",
    processKeyPointsInMainThread: false, //Add <script async src="/js/slam.js"></script> to slamDemo.html if enabling this
};
