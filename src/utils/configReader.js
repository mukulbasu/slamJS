/**
 * @file configReader.js
 * @brief File for extracting configs from js file and logging it to screen
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
const execSync = require('child_process').execSync

const configFile = process.argv[2];
const config = require("../config/"+configFile+".js");

for (key in config) {
    console.log(key);
    if (Array.isArray(config[key])) {
        console.log(config[key].join("\n"));
    } else {
        console.log(config[key]);
    }
    console.log("DATA_END");
}