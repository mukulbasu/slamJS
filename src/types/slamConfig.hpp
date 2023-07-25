/**
 * @file slamConfig.hpp
 * @brief Config class to easily tweak various configurations for experimentation and testing.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __SLAM_CONFIG_HPP__
#define __SLAM_CONFIG_HPP__

#include "../utils/configReader.hpp"

#define SET(TYPE, VAR) TYPE VAR = cfg->read_##TYPE(#VAR)

class SlamConfig {
    public:
        ConfigReader* cfg = NULL;
        //BA config
        SET(int, pathStart);
        SET(int, pathEnd);
        SET(int, cx);
        SET(int, cy);
        SET(int, fx);
        SET(int, fy);
        SET(int, maxDepth);

        SET(bool, matchHierarchy);
        SET(int, leafSize);
        SET(int, branchSize);
        SET(int, treeSize);

        //KP config
        SET(int, reqdKpsInit);
        SET(int, reqdKps);

        //Matcher config
        SET(int, maxGap);
        SET(int, minGap);
        SET(float, minAvgGapInit);
        SET(float, minAvgGap);
        SET(double, distanceThreshold);
        SET(float, ratio);
        SET(float, imgWidthRatio);
        SET(bool, debugEstimateValidation);


        SET(int, baOption);
        SET(int, maxFrames);
        SET(int, mapInitializationFrames);
        SET(int, numKeyFrameMatches);
        SET(float, maxDistRatio);
        SET(float, maxAngle);
        SET(bool, copyRotation);
        SET(float, scale);
        SET(bool, findFocus);
        SET(bool, normalizeKP);
        SET(int, debugFrameId);
        SET(bool, disableRotationInput);
        SET(bool, newKeyframesBA);
        SET(float, smootheningTolerance);
        SET(bool, cholmod);

        SlamConfig(ConfigReader* cfgArg) : cfg(cfgArg) {}
};

#endif /* __SLAM_CONFIG_HPP__ */