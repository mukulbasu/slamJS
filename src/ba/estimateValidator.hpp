/**
 * @file estimateValidator.hpp
 * @brief Evaluates the quality of BA estimation.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __ESTIMATE_VALIDATOR_HPP__
#define __ESTIMATE_VALIDATOR_HPP__

// for std
#include <iostream>
#include <map>
// for opencv 
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <boost/concept_check.hpp>
// for g2o
#include "abstractBundleAdjuster.hpp"
#include "../utils/timer.hpp"
#include "../types/types.hpp"
#include "../managers/landmarkManager.hpp"
#include "../managers/frameManager.hpp"

using namespace std;
using namespace cv;
using namespace Eigen;

class EstimateValidator {
    protected:
        SlamConfig& _slamCfg;
        SP<LandmarkManager> _lm;
        SP<FrameManager> _fm;
        Mat _cameraMatrix;
        Mat _distCoeffs;

        SP<Pose> get_frame_pose(
            SP<FramePoseMap> framePoseMap, 
            SP<Frame> frame)
        {
            if (framePoseMap->count(frame) > 0) return (*framePoseMap)[frame];
            else return frame->pose;
        }

        SP<FpValidResult> validate_fp_inlier(
            SP<BA> ba, 
            SP<FramePoint> fp, 
            SP<Landmark> landmark, 
            Vector3d& transKp, 
            float inlierRange, 
            SP<FrameSet> frameSet,
            bool isFrameFixed, 
            bool isLandmarkFixed,
            SP<FramePoseMap> framePoseMap = nullptr) 
        {
            // DEBUG_COUT(currFrameId<<": "<<fp->frame->id<<" Validating for Frame "<<landmark->id<<", "<<append<<", "<<debug<<endl);
            auto pose = get_frame_pose(framePoseMap, fp->frame);
            auto output = make_shared<FpValidResult>();

            //Where both the landmarks and frame is fixed, their FP is not under
            //evaluation and so, is ignored for our computation
            if (isFrameFixed && isLandmarkFixed) {
                output->result = FIXED;
                return output;
            }

            //Check if FP is eligible
            {
                auto distance = (pose->trans - transKp).norm();
                double minDistanceForCheck = distance/3, maxDistanceForCheck = distance/99;
                bool isTooClose = true, isTooFar = true;

                //The landmark will be useful for predicting current frame's position
                //if it is not too far or too close. If the distance between 2 frames is X,
                //then best landmark points are ones that are within X*K1 and X*K2 distance.
                //Here I have assumed K1 and K2 are 3 and 99 respectively.
                //In this case, the landmark might be connected to more than 2 frames, so we try to 
                //find at least one frame each for satisfying the above conditions.
                //It is fine if the frame satisfying each condition is different.
                for (auto frame : *frameSet) {
                    auto poseDistance = (get_frame_pose(framePoseMap, frame)->trans - pose->trans).norm();
                    if (isTooClose && minDistanceForCheck > poseDistance) isTooClose = false;
                    if (isTooFar && maxDistanceForCheck < poseDistance) isTooFar = false;
                    if (!isTooFar && !isTooClose) break;
                }
                if (isTooClose) {
                    output->result = ValidateResultType::INVALID;
                    output->isTooClose = true;
                    return output;
                }
                if (isTooFar) {
                    output->result = ValidateResultType::INVALID;
                    output->isTooFar = true;
                    return output;
                }
            }

            //Check if FP is behind frame
            {
                tie (output->isBehind, output->behindTrans) = TransformUtils::check_behind_frame(pose, transKp);
                if (output->isBehind) {
                    output->result = ValidateResultType::INVALID;
                    return output;
                }
            }

            //Check if projection is within range
            {
                tie (output->px, output->py) = ba->getProjection(pose, transKp);
                bool inRange = TransformUtils::within_range(
                    fp, 
                    output->px, 
                    output->py, 
                    inlierRange, 
                    _slamCfg.normalizeKP, 
                    _cameraMatrix, 
                    _distCoeffs);
                if (!inRange) {
                    output->result = ValidateResultType::INVALID;
                    output->isWithinRange = false;
                    return output;
                }
            }

            output->result = ValidateResultType::VALID;
            return output;
        }


        tuple<SP<LandmarkResult>, SP<FrameResult>, SP<FpLandmarkResult>, SP<FrameFpLandmarks>> 
        initialize(SP<LandmarkSet> landmarkSet, 
                SP<FrameSet> frameSet,
                SP<LandmarkSet> fixedLandmarks,
                SP<FrameSet> fixedFrames) {
            auto landmarkResult = make_shared<LandmarkResult>();
            auto frameResult = make_shared<FrameResult>();
            auto fpLandmarkResult = make_shared<FpLandmarkResult>();
            auto frameFpLs = make_shared<FrameFpLandmarks>();

            for (auto f : *frameSet) (*frameFpLs)[f] = make_shared<FramePointLandmarkPairSet>();

            for (auto landmark : *landmarkSet) {
                if (fixedLandmarks->count(landmark) == 0) {
                    landmarkResult->put(landmark, UNSET);
                    // DEBUG_COUT("Set Landmark "<<landmark->id<<": UNSET"<<endl);
                } else {
                    landmarkResult->put(landmark, FIXED);
                    // DEBUG_COUT("Set Landmark "<<landmark->id<<": FIXED"<<endl);
                }

                auto fpValidResult = make_shared<FpValidResult>();
                fpValidResult->result = UNSET;
                for (auto fp : landmark->fps) {
                    if (frameSet->count(fp->frame) == 0) continue;
                    (*fpLandmarkResult)[fp][landmark] = fpValidResult;
                    (*frameFpLs)[fp->frame]->insert(make_pair(fp, landmark));
                }
            }
            
            FrameSet frames;
            for (auto& [fp, lFpValidResults] : *fpLandmarkResult) 
                frames.insert(fp->frame);
            
            for (auto frame : frames) {
                if (fixedFrames->count(frame) == 0) {
                    frameResult->put(frame, UNSET);
                    // DEBUG_COUT("Set Frame "<<frame->id<<": UNSET"<<endl);
                } else {
                    frameResult->put(frame, FIXED);
                    // DEBUG_COUT("Set Frame "<<frame->id<<": FIXED"<<endl);
                }
            }

            return make_tuple(landmarkResult, frameResult, fpLandmarkResult, frameFpLs);
        }

        tuple<int, int> count_inliers(
                SP<FramePointLandmarkPairSet> fpLandmarkPairs,
                SP<FpLandmarkResult> fpLandmarkResult,
                SP<LandmarkResult> landmarkResult) {
            int inlier = 0, outlier = 0;
            for (auto& [fp, landmark] : *fpLandmarkPairs) {
                auto result = landmarkResult->get(landmark);
                if (result == VALID || result == FIXED) {
                    auto assessment = (*fpLandmarkResult)[fp][landmark];
                    if (assessment->result == VALID) {
                        inlier++;
                    } else if (assessment->result == INVALID) {
                        outlier++;
                    }
                }
            }
            return make_tuple(inlier, outlier);
        }
    
    public:
        EstimateValidator(
            SlamConfig& slamCfg, 
            SP<LandmarkManager> lm, 
            SP<FrameManager> fm, 
            const Mat& cameraMatrix, 
            const Mat& distCoeffs) : 
            _slamCfg(slamCfg), 
            _lm(lm), 
            _fm(fm), 
            _cameraMatrix(cameraMatrix), 
            _distCoeffs(distCoeffs) {}

        ~EstimateValidator() {}

        /**
         * @brief Evaluates the quality of estimate generated by BA.
         * To do so, we start with the fixed frames and fixed landmarks. The fixed landmarks are assumed valid.
         * The landmarks with correct projection on fixed frames are assumed valid too.
         * Next, we extract the frames that have > "poseEstimateThreshold"
         * connections to valid landmarks. If the number of inliers among valid landmarks for these frames is >
         * "goodLandmarkRatio", then the frame is deemed valid, else invalid. 
         * For the newly valid frames, all the other landmarks connected to them are evaluated and marked valid
         * if the connection is an inlier. 
         * Based on the updated list of valid landmarks, frames connected to them are extracted and evaluated like before.
         * 
         * @param ba 
         * @param currFrame 
         * @param landmarkSet 
         * @param fixedLandmarks 
         * @param fixedFrames 
         * @param landmarkTransMap 
         * @param framePoseMap 
         * @param ransacId 
         * @param debug 
         * @param append 
         * @return SP<ValidatorOutput> 
         */
        SP<ValidatorOutput> validate_estimates(SP<BA> ba,
                SP<LandmarkSet> landmarkSet, 
                SP<FrameSet> frameSet,
                SP<LandmarkSet> fixedLandmarks,
                SP<FrameSet> fixedFrames,
                SP<map<SP<Frame>, int>> frameRank,
                int maxRank,
                float inlierRange,
                float goodLandmarkRatio,
                float goodFrameRatio,
                float goodAvgInlierRatio,
                SP<LandmarkTransMap> landmarkTransMap,
                SP<FramePoseMap> framePoseMap,
                bool validate = true) {
            float totalFrameInlierRatios = 0;
            
            auto [landmarkResult, frameResult, fpLandmarkResult, frameFpLPairs] =
                initialize(landmarkSet, frameSet, fixedLandmarks, fixedFrames);

            DEBUG_COUT("Initialized"<<endl);

            if (validate) {
                //For the FPs under evaluation, figure out which ones are valid
                for (auto& [fp, lFpValidResult] : *fpLandmarkResult) {
                    auto isFrameFixed = fixedFrames->count(fp->frame) > 0;
                    for (auto& [landmark, fpValidResult] : lFpValidResult) {
                        auto isLandmarkFixed = fixedLandmarks->count(landmark) > 0;
                        auto assessment = validate_fp_inlier(
                            ba, 
                            fp, 
                            landmark, 
                            (*landmarkTransMap)[landmark], 
                            inlierRange, 
                            frameSet, 
                            isFrameFixed, 
                            isLandmarkFixed,
                            framePoseMap);
                        (*fpLandmarkResult)[fp][landmark] = assessment;
                        // DEBUG_COUT("Set FP "<<fp->id<<":"<<landmark->id<<": "<<assessment->result<<endl);

                        //If the FP comes to true or false against a fixed frame 
                        // set the landmarkResult. Also even if landmark is invalid against one 
                        // fixed frame but valid against another fixed frame, the VALID result
                        // will override the invalid one.
                        if (isFrameFixed && (landmarkResult->get(landmark) == UNSET ||\
                            landmarkResult->get(landmark) == INVALID)) {
                            if (assessment->result == VALID) {
                                landmarkResult->put(landmark, assessment->result);
                                // DEBUG_COUT("Set Landmark "<<landmark->id<<": "<<assessment->result<<endl);
                            }
                        }
                    }
                }
                
                DEBUG_COUT("FP Assessment Completed"<<endl);

                
                //Process for fixed frames
                for (auto frame : *frameResult->get(FIXED)) {
                    auto fpLPairs = (*frameFpLPairs)[frame];
                    int inlier = 0, outlier = 0;
                    for (auto& [fp, landmark] : *fpLPairs) {
                        if (landmarkResult->get(landmark) == FIXED) continue;
                        auto assessment = (*fpLandmarkResult)[fp][landmark];
                        if (assessment->result == VALID) {
                            inlier++;
                        } else if (assessment->result == INVALID) {
                            outlier++;
                        }
                    }
                    
                    DEBUG_COUT("Frame "<<frame->id<<" Inliers "<<inlier<<" Outlier "<<outlier<<endl);
                    assert(inlier+outlier >= 0);
                    float inlierRatio = ((float) inlier )/ (inlier + outlier);
                    totalFrameInlierRatios += inlierRatio;
                }
                DEBUG_COUT("Fixed Frames processed"<<endl);

                int rankUnderConsideration = 0;

                while (rankUnderConsideration < maxRank) {
                    FrameSet newValidFrames;
                    for (auto& [frame, rank] : *frameRank) {
                        // DEBUG_COUT("Considering "<<frame->id<<": "<<rank<<", "<<rankUnderConsideration<<endl);
                        if (rank != rankUnderConsideration) continue;
                        // DEBUG_COUT("Checking unset "<<frameResult->get(frame)<<endl);
                        if (frameResult->get(frame) != UNSET) continue;
                        auto fpLPairs = (*frameFpLPairs)[frame];
                        auto [inlier, outlier] = count_inliers(
                            fpLPairs, 
                            fpLandmarkResult,
                            landmarkResult);
                        DEBUG_COUT("Frame "<<frame->id<<" Inliers "<<inlier<<" Outlier "<<outlier<<endl);
                        assert(inlier+outlier >= 0);
                        float inlierRatio = ((float) inlier )/ (inlier + outlier);
                        if (inlierRatio >= goodLandmarkRatio) {
                            totalFrameInlierRatios += inlierRatio;
                            frameResult->put(frame, VALID);
                            newValidFrames.insert(frame);
                            // DEBUG_COUT("Set Frame "<<frame->id<<": VALID"<<endl);
                        } else {
                            frameResult->put(frame, INVALID);
                            // DEBUG_COUT("Set Frame "<<frame->id<<": INVALID"<<endl);
                        }
                    }

                    for (auto frame : newValidFrames) {
                        for (auto& [fp, landmark] : *(*frameFpLPairs)[frame]) {
                            auto result = (*fpLandmarkResult)[fp][landmark]->result;
                            if (result == VALID || result == FIXED) {
                                landmarkResult->put(landmark, VALID);
                                // DEBUG_COUT("Set Landmark "<<landmark->id<<": VALID"<<endl);
                            }
                        }
                    }
                    DEBUG_COUT("Rank "<<rankUnderConsideration<<" processed"<<endl);
                    rankUnderConsideration++;
                }
            }

            for (auto landmark : *landmarkSet) {
                if (UNSET == landmarkResult->get(landmark))
                    landmarkResult->put(landmark, INVALID);
            }
            DEBUG_COUT("All Ranks processed"<<endl);

            
            float avgInlierRatio = totalFrameInlierRatios / (frameResult->size(VALID) + frameResult->size(FIXED));
            float validFrameRatio = frameResult->size() == frameResult->size(FIXED)? 1 : 
                        ((float)frameResult->size(VALID)) / (frameResult->size() - frameResult->size(FIXED));

            if (_slamCfg.debugEstimateValidation) {
                LOG("AvgInlierRatio "<<avgInlierRatio<<" vs "<<goodAvgInlierRatio);
                LOG(", ValidFrameRatio "<<validFrameRatio<<" vs "<<goodFrameRatio);
                LOG(" FrameResult sizes "<<frameResult->size()<<", "<<frameResult->size(VALID)<<", "<<frameResult->size(FIXED));
                LOG(endl);
            }

            bool isValid = validFrameRatio >= goodFrameRatio && avgInlierRatio >= goodAvgInlierRatio;

            return make_shared<ValidatorOutput>(
                    landmarkTransMap, framePoseMap, 
                    landmarkResult, frameResult, fpLandmarkResult,
                    avgInlierRatio, validFrameRatio, isValid);
        }

        static void replace_landmark(
            SP<ValidatorOutput> vo,
            SP<Landmark> orig, 
            SP<Landmark> landmark) 
        {
            if (orig == landmark) return;
            auto& fpLandmarkResult = vo->fpLandmarkResult;
            auto& landmarkResult = vo->landmarkResult;

            landmarkResult->replace(orig, landmark);

            FramePointSet fps;
            fps.insert(orig->fps.begin(), orig->fps.end());
            fps.insert(landmark->fps.begin(), landmark->fps.end());
            
            for (auto& [fp, lFpValid] : *vo->fpLandmarkResult) {
                if (lFpValid.count(orig) > 0) {
                    lFpValid[landmark] = lFpValid[orig];
                    lFpValid.erase(orig);
                    // DEBUG_COUT(orig->id<<":"<<fp->id<<":"<<fp->frame->id<<": Has been erased "<<fp->id<<endl);
                }
            }
            // DEBUG_COUT("fpLandmarkResult has been replaced"<<endl);

            if (vo->landmarkTransMap->count(orig) > 0) {
                (*vo->landmarkTransMap)[landmark] = (*vo->landmarkTransMap)[orig];
                vo->landmarkTransMap->erase(orig);
            }

            for (auto fp : landmark->fps) {
                if (orig->fps.count(fp) == 0) {
                    //This is a new FP. Set it to UNSET
                    auto output = make_shared<FpValidResult>();
                    output->result = UNSET;
                    (*fpLandmarkResult)[fp][landmark] = output;
                }
            }
        }
};

#endif /* __ESTIMATE_VALIDATOR_HPP__ */