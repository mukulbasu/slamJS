/**
 * @file poseManager.hpp
 * @brief Main class that computes the pose of every new camera frame
 * by leveraging frame manager, landmark manager, matcher, baHelper etc.
 * The key methods are
 * 1. add_frame: Used to add new camera frames as they are generated. The 
 * computed estimate is written into the frame object.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __POSE_MANAGER_HPP__
#define __POSE_MANAGER_HPP__

// for std
#include <iostream>
#include <map>
// for opencv 
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <boost/concept_check.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/calib3d/calib3d_c.h>

#include <opencv2/core/core_c.h>
// for g2o

#include "../utils/timer.hpp"
#include "matcher.hpp"
#include "baHelper.hpp"

using namespace std;
using namespace cv;
using namespace Eigen;

class PoseManager {
    protected:
        SlamConfig& _cfg;
        double _scale = 1.0;
        int _startFrameId = 0;
        int _stopFrameId = 0;
        SP<LandmarkManager> _lm;
        SP<Matcher> _matcher;
        SP<FrameManager> _fm;
        bool _initialized = false;
        cv::Mat _cameraMatrix = (cv::Mat_<double>(3, 3) << 466, 0, 0, 0, 466, 0, 0, 0, 1);//cv::Mat::eye(3, 3, CV_64F); 
        cv::Mat _distCoeffs = (cv::Mat_<double>(5, 1) << -0.00384385, 0.00176262, -0.00070753, -0.00131189,  -0.0103289);
        // cv::Mat _distCoeffs = (cv::Mat_<double>(5, 1) << 0.0, 0.0, 0.0, 0.0, 0.0);
        // cv::Mat _distCoeffs = (cv::Mat_<double>(5, 1) << 0.214, -0.855, -0.001, -0.00313,  1.099);


        SP<BaHelper> generate_ba_helper(SP<Frame> currFrame) 
        {
            return make_shared<BaHelper>(_cfg, 
                currFrame, 
                _fm, 
                _lm, 
                _cameraMatrix, 
                _distCoeffs);
        }

        SP<BaHelper> generate_ba_helper(
            SP<Frame> currFrame, 
            double fx) 
        {
            cv::Mat cameraMatrix = (cv::Mat_<double>(3, 3) << fx, 0, 0, 0, fx, 0, 0, 0, 1);
            return make_shared<BaHelper>(_cfg, 
                currFrame, 
                _fm, 
                _lm,
                cameraMatrix, 
                _distCoeffs);
        }

        /**
         * @brief Helper method to merge new landmarks created for matching
         * with existing landmarks, dedupe landmarks etc.
         * 
         * @param currFrame 
         * @param goodLandmarks 
         * @param replacements - Replaced landmarks are placed in this holder
         */
        void add_to_landmarks(
            SP<Frame> currFrame, 
            SP<LandmarkSet> goodLandmarks, 
            SP<LandmarkPairVec> replacements) 
        {
            LandmarkSet set(*goodLandmarks);
            for (auto goodLandmark : set) {
                SP<Landmark> landmark;
                // DEBUG_COUT("---------------"<<endl);
                for (auto fp : goodLandmark->fps) {
                    if (!landmark) {
                        if (fp->landmark.expired()) {
                            // DEBUG_COUT("Creating fresh landmark"<<endl);
                            landmark = _lm->create_landmark(fp, 0);
                        } else {
                            // DEBUG_COUT("Existing landmark"<<endl);
                            landmark = fp->landmark.lock();
                        }
                    } else {
                        if (fp->landmark.expired()) {
                            _lm->link_landmark_point(landmark, fp, 0);
                        } else {
                            auto existingLandmark = fp->landmark.lock();
                            if (landmark != existingLandmark) {
                                DEBUG_COUT(currFrame->id<<":Merging landmark "<<existingLandmark->id<<", "<<landmark->id<<endl);
                                _lm->merge_landmarks(landmark, existingLandmark);
                                replacements->push_back(make_pair(existingLandmark, landmark));
                            }
                        }
                    }
                }
                if (landmark) {
                    _lm->dedupe_landmark_points(landmark);
                    if (goodLandmark != landmark) {
                        replacements->push_back(make_pair(goodLandmark, landmark));
                    }
                    if (!landmark->valid) {
                        // DEBUG_COUT("Good Landmark trans "<<goodLandmark->trans[0]<<", ");
                        // DEBUG_COUT(goodLandmark->trans[1]<<", "<<goodLandmark->trans[2]<<endl); 
                        landmark->trans[0] = goodLandmark->trans[0];
                        landmark->trans[1] = goodLandmark->trans[1];
                        landmark->trans[2] = goodLandmark->trans[2];
                    }
                }
            }
        }

        /**
         * @brief Standardizes the output scale of SLAM
         * 
         * @param startFrameId 
         * @param stopFrameId 
         * @param scale 
         */
        void right_scale(int startFrameId, int stopFrameId, float scale) {
            double totalDistance = 0;
            SP<Frame> lastFrame = nullptr;
            auto keyframes = _fm->get_keyframes();
            for (auto frame : *keyframes) {
                if (frame->id >= startFrameId && frame->id <= stopFrameId) {
                    if (lastFrame) {
                        totalDistance += abs(frame->pose->trans[0] - lastFrame->pose->trans[0]);
                    }
                    lastFrame = frame;
                }
            }
            double scaleFactor = scale / totalDistance;
            DEBUG_COUT("Scale Factor "<<totalDistance<<", "<<scaleFactor<<endl);

            for (auto frame : *keyframes) {
                auto& trans = frame->pose->trans;
                for (int i = 0; i<3; i++) trans[i] = trans[i] * scaleFactor;
            }

            for (auto frame : *_fm->frameList) {
                if (keyframes->count(frame) > 0) continue;
                auto& trans = frame->pose->trans;
                for (int i = 0; i<3; i++) trans[i] = trans[i] * scaleFactor;
            }
            for (auto landmark : *_lm->get_landmarks()) {
                auto& trans = landmark->trans;
                for (int i = 0; i<3; i++) trans[i] = trans[i] * scaleFactor;
            }
        }

        /**
         * @brief Computes the degree and distance difference between 2 frames
         * 
         * @param currFrame 
         * @param matchFrame 
         * @return tuple<double, double, double> 
         */
        tuple<double, double, double> compute_diff(
            SP<Frame> currFrame, 
            SP<Frame> matchFrame) 
        {
            //Get the distance as a ratio of frame->landmarkDist;
            auto dist = (currFrame->pose->trans - matchFrame->pose->trans).norm();
            auto distRatio = dist / matchFrame->landmarkDistThreshold;
            cout<<"DIst Threshold "<<currFrame->id<<" vs "<<matchFrame->id<<": "<<dist <<", "<< matchFrame->landmarkDistThreshold<<endl;
            //Get the angle difference
            auto degDiff = TransformUtils::deg_diff(currFrame->pose->rot, 
                    matchFrame->pose->rot);
            return make_tuple(degDiff, distRatio, dist);
        }

        /**
         * @brief Extracts eligible match frames from frameset based on degree and distance
         * difference
         * 
         * @param currFrame 
         * @param frameSet 
         * @param mult 
         * @return SP<FrameSortVec> 
         */
        SP<FrameSortVec> generate_keyframe_ranks(
            SP<Frame> currFrame, 
            FrameSet& frameSet, 
            float mult)
        {
            auto frameSortVec = make_shared<FrameSortVec>();
            for (auto matchFrame : frameSet) {
                if (false == matchFrame->valid) continue;

                auto [degDiff, distRatio, dist] = compute_diff(currFrame, matchFrame);
                // DEBUG_COUT(currFrame->id<<": Dist Ratio and Deg Diff ");
                // DEBUG_COUT(matchFrame->id<<": "<<distRatio<<", "<<degDiff<<endl);
                if (degDiff < _cfg.maxAngle*mult && degDiff > (-_cfg.maxAngle*mult)) {
                    if (distRatio < _cfg.maxDistRatio*mult) {
                        frameSortVec->push_back(make_shared<FrameSort>(matchFrame, degDiff, distRatio, dist));
                    } 
                }
            }
            std::sort(frameSortVec->begin(), frameSortVec->end(), [] (const auto& a, const auto& b) {
                if (abs(a->degDiff - b->degDiff) < 15) {
                    //if the camera of both frames is pointing in the same direction
                    //then it is better to prioritise matching with a frame that is a 
                    //little distant from the current one, to triangulate landmarks better
                    return a->dist > b->dist;
                } else {
                    if (a->distRatio > 0.1 && b->distRatio <= 0.1) {
                        return false;
                    } else if (a->distRatio <= 0.1 && b->distRatio > 0.1) {
                        return true;
                    //since the angle is already more than 15 deg
                    //it is better to pick frames that are closer
                    //so that there is more chance of viewport overlap
                    } else return a->dist < b->dist;
                }
            });
            return frameSortVec;
        }

        /**
         * @brief Generates suitable match frames for current frame from the keyframes and past frames.
         * 
         * @param currFrame 
         * @return SP<FrameSet> 
         */
        SP<FrameSet> get_match_frames(SP<Frame> currFrame) {
            auto matchFrames = make_shared<FrameSet>();
            if (!_initialized) {
                matchFrames->insert(_fm->originFrame);
            } else {
                SP<FrameSortVec> keyFrameRanks, prevFrameRanks;
                auto keyframes = _fm->get_keyframes();
                FrameSet prevFrameSet;
                for (auto frame : *_fm->frameList) {
                    if (currFrame == frame || keyframes->count(frame) > 0) continue;
                    prevFrameSet.insert(frame);
                }
                keyFrameRanks = generate_keyframe_ranks(currFrame, *keyframes, 1.0);
                if ((int)keyFrameRanks->size() > 0) {
                    matchFrames->insert(keyFrameRanks->at(0)->frame);
                    keyFrameRanks->erase(keyFrameRanks->begin());
                }
                for (int i = 0; (int)matchFrames->size() < _cfg.numKeyFrameMatches && i < (int)keyFrameRanks->size(); i++) {
                    matchFrames->insert(TransformUtils::pop<SP<FrameSort>>(
                            *keyFrameRanks, keyFrameRanks->size()/2)->frame);
                }

                if ((int)matchFrames->size() < _cfg.numKeyFrameMatches) {
                    prevFrameRanks = generate_keyframe_ranks(currFrame, prevFrameSet, 1.0);
                    for (int i = 0; (int)matchFrames->size() < _cfg.numKeyFrameMatches && i < (int)prevFrameRanks->size(); i++) {
                        matchFrames->insert(prevFrameRanks->at(i)->frame);
                    }
                    
                    if ((int)matchFrames->size() < _cfg.numKeyFrameMatches) {
                        keyFrameRanks = generate_keyframe_ranks(currFrame, *keyframes, 2.0);
                        for (int i = 0; (int)matchFrames->size() < _cfg.numKeyFrameMatches &&\
                                i < (int)keyFrameRanks->size(); i++) {
                            matchFrames->insert(keyFrameRanks->at(i)->frame);
                        }
                        if ((int)matchFrames->size() < _cfg.numKeyFrameMatches) {
                            prevFrameRanks = generate_keyframe_ranks(currFrame, prevFrameSet, 2.0);
                            for (int i = 0; (int)matchFrames->size() < _cfg.numKeyFrameMatches &&\
                                    i < (int)prevFrameRanks->size(); i++) {
                                matchFrames->insert(prevFrameRanks->at(i)->frame);
                            }
                        }
                    }
                }

            }
            return matchFrames;
        }

        /**
         * @brief Computes focus based on available keyframes
         * 
         * @param currFrame 
         * @param focusStart 
         * @param focusEnd 
         * @param divisions 
         * @return int 
         */
        int find_focus(SP<Frame> currFrame, int focusStart, int focusEnd, int divisions) {
            auto goodLandmarks = make_shared<LandmarkSet>();
            for (auto frame : *_fm->get_keyframes()) {
                for (auto fp : frame->fps) {
                    auto landmark = fp->landmark.lock();
                    if (landmark && landmark->valid) {
                        goodLandmarks->insert(landmark);
                    }
                }
            }
            // auto focuses = vector<int>{250, 255, 260, 265, 270, 275};
            // auto focuses = vector<int>{200, 225, 250, 275, 300, 325, 350, 375, 400, 425, 450, 475, 500};
            auto focuses = vector<int>{240, 245, 250, 255, 260, 265, 270, 275, 280, 285, 290};
            // auto focuses = vector<int>{270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 280};
            float minError = 9999999;
            int selectedFocus = 0;
            auto fixedFrames = make_shared<FrameSet>();
            fixedFrames->insert(_fm->originFrame);
            for (int focus = focusStart; focus <= focusEnd; 
                    focus += (focusEnd - focusStart)/divisions) {
            // for (auto focus : focuses) {
                auto baHelper = generate_ba_helper(currFrame, focus);
                auto result = baHelper->estimate(
                    goodLandmarks, 
                    _fm->get_keyframes(),
                    make_shared<LandmarkSet>(), 
                    fixedFrames, 
                    30, 1, 0.5, 1.0, 0.7);
                if (result && result->validatorOutput->valid) {
                    auto error = baHelper->get_error();
                    // DEBUG_COUT("Running iteration on Focus "<<focus<<" Error is ");
                    // DEBUG_COUT(error<<", Error * focus "<<error*focus<<endl);
                    if (minError > error*focus) {
                        minError = error*focus;
                        selectedFocus = focus;
                    }
                } else {
                    cout<<"No valid result was found for focus"<<endl;
                }
            }
            if (selectedFocus == 0) {
                cout<<"No focus found"<<endl;
                return -1;
            } else {
                // cout<<currFrame->id<<": Selected Focus is "<<selectedFocus<<" minError is "<<minError<<endl;
                return selectedFocus;
            }
        }


    public:
        PoseManager(
            SlamConfig& cfg, 
            SP<LandmarkManager> lm, 
            SP<Matcher> matcher, 
            SP<FrameManager> fm
        ) : _cfg(cfg), 
            _lm(lm), 
            _matcher(matcher), 
            _fm(fm)
        {
            _cameraMatrix.at<double>(0, 0) = _cfg.fx;
            _cameraMatrix.at<double>(1, 1) = _cfg.fy;
        }

        ~PoseManager() {
        }

        /**
         * @brief Key method for pose computation.
         * The pose computation is done in phases:
         * 1. Match frames are identified
         * 2. For each match frame, Framepoints are matched to currFrame using descriptors and matches (landmarks) are generated 
         * 3. RANSAC is done on a small subset of matches to arrive at frame position
         * 4. Each RANSAC is evaluated against another subset of test matches to decide winner
         * 5. All matches are evaluated against the winner frame position to extract valid matches
         * 6. Valid matches are used for final frame position computation
         * 
         * @param currFrame 
         * @return SP<PoseManagerOutput> 
         */
        SP<PoseManagerOutput> add_frame(SP<Frame> currFrame) 
        {
            DEBUG_COUT(currFrame->id<<":"<<LOG_START<<endl);
            cout<<"Add Frame"<<endl;
            auto output = make_shared<PoseManagerOutput>();
            output->frame = currFrame;
            for (int i = 0; i < 4; i++) {
                output->results.push_back(vector<SP<BaHelperOutput>>());
            }

            if (_fm->frameList->size() == 1) {
                _fm->add_keyframe(currFrame);
                currFrame->level = 0;
                currFrame->valid = true;
                output->valid = true;
                auto originTrans = _fm->set_origin_frame(currFrame);
                _lm->set_origin_trans(originTrans);
                DEBUG_COUT(currFrame->id<<":"<<LOG_END<<endl);
                return output;
            }

            Timer frameExtTimer, matchTimer, ransacTimer, winnerTimer, validTimer, poseTimer;
            
            //Figure out keyframes that might be a suitable match.
            frameExtTimer.start();
            auto analysisStartTime = Timer::time();
            auto matchFrames = get_match_frames(currFrame);
            if (!matchFrames || matchFrames->size() == 0) {
                cout<<"Not enough matchframes to proceed"<<endl;
                output->status = NOT_ENOUGH_MATCH_FRAMES;
                DEBUG_COUT(currFrame->id<<":"<<LOG_END<<endl);
                return output;
            }
            output->matchFrames->insert(matchFrames->begin(), matchFrames->end());
            frameExtTimer.stop();

            cout<<currFrame->id<<": Match Frames extracted "<<matchFrames->size()<<endl;
            auto sortTime = Timer::diff(analysisStartTime);
            analysisStartTime = Timer::time();

            //Generate Ransac set
            vector<tuple<SP<BaHelperOutput>, SP<BaHelper>>> ransacResults;
            
            {
                // map<SP<Frame>, FramePointVec> frameFps;
                map<SP<Frame>, LandmarkVec> frameMatches;
                int ransacMatchSize = 6;
                if (ransacMatchSize < (int)matchFrames->size()) 
                    ransacMatchSize = matchFrames->size();
                int ransacIters = (!_initialized? 18 : 12);
                int totalMatches = ransacMatchSize * ransacIters;
                int maxMatchesPerFrame = totalMatches/(int)matchFrames->size();
                int maxMatchesPerFramePerIter = maxMatchesPerFrame/ransacIters;

                matchTimer.start();
                FrameVec badFrames;
                auto descriptorFrames = make_shared<FrameSet>();
                descriptorFrames->insert(currFrame);
                descriptorFrames->insert(matchFrames->begin(), matchFrames->end());
                descriptorFrames->insert(_fm->get_keyframes()->begin(), _fm->get_keyframes()->end());

                for (auto frame : *matchFrames) {
                    auto matchSet = _matcher->match_fps(
                        frame, 
                        currFrame->fps, 
                        descriptorFrames, 
                        maxMatchesPerFrame, 
                        _initialized? _cfg.minAvgGap : _cfg.minAvgGapInit);
                    if (matchSet->size() > 3) {
                        for (auto l : *matchSet) frameMatches[frame].push_back(l);
                        for (int i = 0; i < maxMatchesPerFrame - (int)matchSet->size(); i++) {
                           frameMatches[frame].push_back(frameMatches[frame][rand() % matchSet->size()]);
                        }
                    } else {
                        cout<<currFrame->id<<": Not enough matches from "<<frame->id<<endl;
                        badFrames.push_back(frame);
                    }
                }

                //Remove frames that do not have matches from the matchFrames list
                for (auto frame : badFrames) {
                    cout<<"Erasing frame "<<frame->id<<endl;
                    matchFrames->erase(frame);
                }

                //If map has been initialized, then min number of match frames are needed for scale
                if (_initialized && (int)matchFrames->size() < 2) {
                    cout<<"Not enough matchframes to proceed"<<endl;
                    output->status = NOT_ENOUGH_MATCH_FRAMES;
                    DEBUG_COUT(currFrame->id<<":"<<LOG_END<<endl);
                    return output;
                }

                auto frameSet = make_shared<FrameSet>();
                frameSet->insert(currFrame);
                frameSet->insert(matchFrames->begin(), matchFrames->end());
                for (auto frame : *matchFrames) _fm->add_keyframe(frame);

                matchTimer.stop();
                        
                        
                DEBUG_COUT(currFrame->id<<":0:"<<LOG_START<<endl);
                ransacTimer.start();
                for (int i = 0; i < ransacIters/2; i++) {
                    DEBUG_COUT(currFrame->id<<":0:"<<i<<":"<<LOG_START<<endl);
                    auto ransacSet = make_shared<LandmarkSet>();
                    for (auto frame : *matchFrames) {
                        for (int j = 0; j < maxMatchesPerFramePerIter;j++) {
                            // DEBUG_COUT("Frame Matches "<<frame->id<<" ransac iter "<<i<<" ransacMatchSize "<<ransacMatchSize<<" match frame size "<<matchFrames->size()<<" j "<<j<<endl);
                            // DEBUG_COUT(" frameMatches[frame] size "<<frameMatches[frame].size()<<endl);
                            // DEBUG_COUT(" maxMatchesPerFrame "<<maxMatchesPerFrame<<" totalMatches "<<totalMatches<<endl);
                            // DEBUG_COUT(" maxMatchesPerFramePerIter "<<maxMatchesPerFramePerIter<<endl);
                            ransacSet->insert(frameMatches[frame][i * maxMatchesPerFramePerIter+j]);
                        }
                    }

                    //Run ransac
                    {
                        auto baHelper = generate_ba_helper(currFrame);
                        auto result = baHelper->estimate(
                            ransacSet,
                            frameSet,
                            nullptr,
                            matchFrames,
                            9, 3*_cfg.imgWidthRatio, 0.5, 1.0, 0.7,
                            false
                        );
                        ransacResults.push_back(make_tuple(result, baHelper));
                        output->results[0].push_back(result);
                    }
                    DEBUG_COUT(currFrame->id<<":0:"<<i<<":"<<LOG_END<<endl);
                }
                
                ransacTimer.stop();
                DEBUG_COUT(currFrame->id<<":0:"<<LOG_END<<endl);
                auto initRansacTime = Timer::diff(analysisStartTime);
                analysisStartTime = Timer::time();
                DEBUG_COUT("Post Init RANSAC "<<endl);

                DEBUG_COUT(currFrame->id<<":1:"<<LOG_START<<endl);
                //Extract pool winners
                SP<BaHelperOutput> bestResult;
                SP<BaHelper> bestBaHelper;

                winnerTimer.start();
                auto evalSet = make_shared<LandmarkSet>();
                for (auto& [frame, matches] : frameMatches) {
                    LandmarkSet ransacSet;
                    ransacSet.insert(matches.begin(), matches.end());
                    int maxLen = ransacSet.size() >= 3? 3 : ransacSet.size();
                    for (int i = 0; i < maxLen; i++) 
                        evalSet->insert(TransformUtils::pop_random<SP<Landmark>>(ransacSet));
                }
                DEBUG_COUT("Eval set prepared, size "<<evalSet->size()<<endl);

                bestResult = nullptr;
                bestBaHelper = nullptr;
                for (int i = 0; i < ransacIters/2; i++) {
                    DEBUG_COUT(currFrame->id<<":1:"<<i<<":"<<LOG_START<<endl);
                    auto [result, baHelper] = ransacResults[i];
                    baHelper = generate_ba_helper(currFrame);
                    result = baHelper->estimate(
                        evalSet,
                        frameSet,
                        result->landmarkSet,
                        frameSet,
                        3, 3*_cfg.imgWidthRatio, 0.5, 0.0, 0.7,
                        true,
                        result->validatorOutput->landmarkTransMap,
                        result->validatorOutput->framePoseMap
                    );
                    output->results[1].push_back(result);
                    bestResult = baHelper->get_best(bestResult, result);
                    if (bestResult == result) {
                        bestBaHelper = baHelper;
                        output->winnerRansacIndex = i;
                    }
                    DEBUG_COUT(currFrame->id<<":1:"<<i<<":"<<LOG_END<<endl);
                }
                
                winnerTimer.stop();
                DEBUG_COUT(currFrame->id<<":1:"<<LOG_END<<endl);
                auto overallWinnerTime = Timer::diff(analysisStartTime);
                analysisStartTime = Timer::time();
                DEBUG_COUT("Post Init All Winner done "<<endl);

                DEBUG_COUT(currFrame->id<<":2:"<<LOG_START<<endl);
                //Complete iterations
                validTimer.start();
                {
                    auto allSet = make_shared<LandmarkSet>();
                    for (auto& [frame, matches] : frameMatches) {
                        LandmarkSet ransacSet;
                        allSet->insert(matches.begin(), matches.end());
                    }

                    DEBUG_COUT(currFrame->id<<":2:"<<0<<":"<<LOG_START<<endl);
                    bestBaHelper = generate_ba_helper(currFrame);
                    bestResult = bestBaHelper->estimate(
                        allSet,
                        frameSet,
                        nullptr,
                        frameSet,
                        9, 10*_cfg.imgWidthRatio, 0.6, 0.0, 0.5,
                        true,
                        bestResult->validatorOutput->landmarkTransMap,
                        bestResult->validatorOutput->framePoseMap
                    );
                    output->results[2].push_back(bestResult);
                    DEBUG_COUT(currFrame->id<<":2:"<<0<<":"<<LOG_END<<endl);
                }
                
                validTimer.stop();
                DEBUG_COUT(currFrame->id<<":2:"<<LOG_END<<endl);
                auto iterationCompleteTime = Timer::diff(analysisStartTime);
                analysisStartTime = Timer::time();
                
                DEBUG_COUT(currFrame->id<<":3:"<<LOG_START<<endl);
                DEBUG_COUT(currFrame->id<<":3:"<<0<<":"<<LOG_START<<endl);
                poseTimer.start();
                {
                    if (bestResult && bestResult->validatorOutput->valid) {
                        auto replacements = make_shared<LandmarkPairVec>();
                        add_to_landmarks(currFrame, 
                            bestResult->validatorOutput->landmarkResult->get(VALID), 
                            replacements);
                        add_to_landmarks(currFrame, 
                            bestResult->validatorOutput->landmarkResult->get(FIXED), 
                            replacements);

                        for (auto& outputVec : output->results) {
                            for (auto baHelperOut : outputVec) {
                                auto vo = baHelperOut->validatorOutput;
                                for (auto [ref, replacement] : *replacements) {
                                    // DEBUG_COUT("Replace "<<ref->id<<" with "<<replacement->id<<endl);
                                    BaHelper::replace_landmark(baHelperOut, ref, replacement);
                                }
                            }
                        }
                        auto vo = bestResult->validatorOutput;
                        auto validLandmarks = vo->landmarkResult->get(VALID);
                        auto fixedLandmarks = vo->landmarkResult->get(FIXED);
                        auto validFrames = vo->frameResult->get(VALID);
                        auto fixedFrames = vo->frameResult->get(FIXED);
                        auto landmarkSet = make_shared<LandmarkSet>();
                        for (auto fp : currFrame->fps) {
                            auto landmark = fp->landmark.lock();
                            if (landmark) {
                                //Check the landmark has at least 2 fp from frameset
                                int fpCnt = 0;
                                for (auto lFp : landmark->fps) {
                                    if (frameSet->count(lFp->frame) > 0) {
                                        fpCnt++;
                                        if (fpCnt >= 2) {
                                            landmarkSet->insert(landmark);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        auto newFrameSet = make_shared<FrameSet>();
                        newFrameSet->insert(frameSet->begin(), frameSet->end());
                        newFrameSet->insert(_fm->get_keyframes()->begin(), _fm->get_keyframes()->end());
                        auto baHelper = generate_ba_helper(currFrame);
                        auto ft = Timer::time();
                        auto result = baHelper->estimate(
                            landmarkSet,
                            newFrameSet,
                            nullptr,
                            _fm->get_keyframes(),
                            18, 10*_cfg.imgWidthRatio, 0.6, 1.0, 0.7,
                            true,
                            bestResult->validatorOutput->landmarkTransMap,
                            bestResult->validatorOutput->framePoseMap
                        );
                        DEBUG_COUT("FT Time "<<Timer::diff(ft)<<endl);
                        assert(result != nullptr);
                        output->results[3].push_back(result);
                        assert(output->results[3].size() > 0);
                        auto poseEstimationTime = Timer::diff(analysisStartTime);
                        DEBUG_COUT("Post Init Pose Estimation "<<endl);

                        if (result->validatorOutput->valid) {
                            DEBUG_COUT("Second estimation passed too");
                            baHelper->copy_estimates(true);
                            _fm->populate_frame_landmark_dist_threshold(newFrameSet);
                            currFrame->valid = true;
                            output->valid = true;
                            output->status = VALID_MATCH;
                            if (!_initialized) {
                                _fm->add_keyframe(currFrame);
                                currFrame->level = 0;
                                _initialized = true;
                                right_scale(0, 100, _cfg.scale);
                                _fm->populate_frame_landmark_dist_threshold(newFrameSet);
                                auto selectedFocus = find_focus(currFrame, 200, 500, 5);
                                LOG("Selected Focus "<<selectedFocus<<endl);
                                if (selectedFocus > 0) {
                                    _cfg.fx = selectedFocus;
                                    _cfg.fy = selectedFocus;
                                    _cameraMatrix.at<double>(0, 0) = selectedFocus;
                                    _cameraMatrix.at<double>(1, 1) = selectedFocus;
                                }
                            }
                            _fm->set_curr_trans_smoothed(currFrame);
                        } else {
                            DEBUG_COUT("Second estimation failed");
                            output->status = MATCH_INVALID;
                        }
                        DEBUG_COUT(currFrame->id<<": Post Init Add Times : Sort "<<sortTime);
                        DEBUG_COUT(" initRansac "<<initRansacTime);
                        DEBUG_COUT(" overallWinner "<<overallWinnerTime);
                        DEBUG_COUT(" iterationComplete "<<iterationCompleteTime);
                        DEBUG_COUT(" poseEstimation "<<poseEstimationTime);
                        DEBUG_COUT(endl);
                    } else {
                        output->status = MATCH_INVALID;
                    }
                }
                poseTimer.stop();
                DEBUG_COUT(currFrame->id<<":3:"<<0<<":"<<LOG_END<<endl);
                DEBUG_COUT(currFrame->id<<":3:"<<LOG_END<<endl);
            }

            output->profile[POSE_FRAME_EXTRACTION_TIME] = frameExtTimer._total;
            output->profile[POSE_MATCH_TIME] = matchTimer._total;
            output->profile[POSE_RANSAC_INIT_TIME] = ransacTimer._total;
            output->profile[POSE_WINNER_TIME] = winnerTimer._total;
            output->profile[POSE_VALID_TIME] = validTimer._total;
            output->profile[POSE_EST_TIME] = poseTimer._total;
            DEBUG_COUT(currFrame->id<<":"<<LOG_END<<endl);
            return output;
        }

        bool is_initialized() {return _initialized;}
};

#endif /* __POSE_MANAGER_HPP__ */