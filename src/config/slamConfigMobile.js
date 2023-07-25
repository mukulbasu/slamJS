/**
 * @file slamConfigModule.js
 * @brief This contains the configurations for running and debugging on local machine.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
const sizeOf = require('image-size');

const pathStart = 1;
const pathEnd = 75;
const dataFolder = "dataTest";
const fileExtension = "png";

const dimensions = sizeOf("data/" + dataFolder + "/image" + pathStart + "." + fileExtension);
const width = dimensions.width;//360
const height = dimensions.height;
const ratio = (width / 480);
const focus = 250 * ratio;//250 is the focus


module.exports = {
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
    debugEstimateValidation: "t",

    pathStart: pathStart.toString(),
    pathEnd: pathEnd.toString(),//190",
    scale: "1",

    baOption: "1",
    maxFrames: "20",
    mapInitializationFrames: "2",
    numKeyFrameMatches: "4",//3
    maxDistRatio: "0.2",
    maxAngle: "30",
    copyRotation: "t",
    findFocus: "t",
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
};
