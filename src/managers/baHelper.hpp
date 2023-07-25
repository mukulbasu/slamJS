/**
 * @file baHelper.hpp
 * @brief Helper class for using BundleAdjustment.
 * Instead of using Bundle Adjustment classes directly, this helper class takes a
 * list of landmarks, fixed frames and fixed landmarks and runs bundle adjustment and 
 * validation on it. The poses, landmarks and framepoints are extracted from the 
 * landmarkSet given as input.
 * The way to use this class:
 * 1. Create an instance of this class
 * 2. Call estimate. This returns BaHelperOutput object.
 * 3. Call copy_estimates
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __BA_HELPER_HPP__
#define __BA_HELPER_HPP__

// for std
#include <iostream>
#include <map>
// for slamjs
#include "../utils/timer.hpp"
#include "../types/types.hpp"
#include "../ba/estimateValidator.hpp"
#include "../ba/bundleAdjuster6Dof.hpp"
#include "../ba/bundleAdjuster3Dof.hpp"
#include "../ba/abstractBundleAdjuster.hpp"

using namespace std;
using namespace Eigen;

#define MIN_ITER_RANSAC 2


class BaHelper {
    protected:
        SlamConfig& _slamCfg;
        SP<Frame> _currFrame;
        SP<LandmarkTransMap> _landmarkEstimateMap = make_shared<LandmarkTransMap>();
        SP<FramePoseMap> _framePoseMap = make_shared<FramePoseMap>();
        SP<EstimateValidator> _estimateValidator;
        SP<FrameManager> _fm;
        SP<LandmarkManager> _lm;
        SP<BaHelperOutput> _output;
        Mat _cameraMatrix;
        Mat _distCoeffs;
        SP<BA> _ba;

        SP<BA> generate_ba(int iterations) 
        {
            double cx = 0;
            double cy = 0;
            double fx = _slamCfg.normalizeKP? 1 : _cameraMatrix.at<double>(0, 0);
            if (_slamCfg.baOption == 0) {
                return make_shared<BundleAdjuster3Dof>(cx, cy, fx,
                    iterations, _slamCfg.maxDepth, _slamCfg.cholmod, _slamCfg);
            } else {
                return make_shared<BundleAdjuster3Dof>(cx, cy, fx,
                    iterations, _slamCfg.maxDepth, _slamCfg.cholmod, _slamCfg);
            }
        }

        tuple<SP<FrameRank>, int> generate_frame_rank(
            SP<LandmarkSet> landmarkSet,
            SP<FrameSet> frameSet,
            SP<LandmarkSet> fixedLandmarks,
            SP<FrameSet> fixedFrames,
            int threshold)
        {
            map<SP<Frame>, set<SP<Landmark>>> fLs;
            map<SP<Landmark>, set<SP<Frame>>> lFs;
            for (auto l : *landmarkSet) {
                for (auto fp : l->fps) {
                    auto f = fp->frame;
                    if (frameSet->count(f) > 0) {
                        fLs[f].insert(l);
                        lFs[l].insert(f);
                    }
                }
            }
            auto spFrameRank = make_shared<FrameRank>();
            LandmarkSet landmarksCovered;
            int rank = 0;
            for (auto f : *fixedFrames) {
                if (frameSet->count(f) > 0) {
                    (*spFrameRank)[f] = rank;
                    landmarksCovered.insert(fLs[f].begin(), fLs[f].end());
                }
            }
            for (auto l : *fixedLandmarks) {
                if (landmarkSet->count(l) > 0) {
                    landmarksCovered.insert(l);
                }
            }

            while (true) {
                rank++;
                FrameSet newFrames;
                for (auto f : *frameSet) {
                    if (spFrameRank->count(f) > 0) continue;
                    int count = 0;
                    for (auto l : fLs[f]) {
                        if (landmarksCovered.count(l) > 0) {
                            count++;
                            if (count >= threshold) break;
                        }
                    }
                    if (count >= threshold) {
                        (*spFrameRank)[f] = rank;
                        newFrames.insert(f);
                    }
                }
                
                if (newFrames.size() == 0) break;

                for (auto f : newFrames) {
                    landmarksCovered.insert(fLs[f].begin(), fLs[f].end());
                }
            }

            DEBUG_COUT("Frame rank "<<spFrameRank->size()<<endl);
            for (auto& [f, rank] : *spFrameRank) DEBUG_COUT(f->id<<": "<<rank<<", ");
            DEBUG_COUT(endl);
            assert (spFrameRank->size() == frameSet->size());
            return make_tuple(spFrameRank, rank);
        }

        tuple<SP<FrameRank>, int> configure_ba_graph(SP<BA> ba,
                SP<LandmarkSet> landmarkSet, 
                SP<FrameSet> frameSet,
                SP<LandmarkSet> fixedLandmarks, 
                SP<FrameSet> fixedFrames,
                SP<LandmarkTransMap> landmarkTransMap = nullptr, 
                SP<FramePoseMap> framePoseMap = nullptr) 
        {
            LandmarkSet landmarksToAdd;
            FrameSet framesToAdd;
            set<pair<SP<FramePoint>, SP<Landmark>>> fpToAdd;
            for (auto const& landmark: *landmarkSet) {
                //If Landmark is already added, skip it.
                if (ba->landmarks.count(landmark)) continue;

                //Check landmark has at least 2 FPS
                int fpLCount = 0;
                for (auto fp : landmark->fps) if (frameSet->count(fp->frame)) fpLCount++;
                if (fpLCount < 2) {
                    DEBUG_COUT("Landmark id "<<landmark->id<<endl);
                }
                assert(fpLCount >= 2);

                //If Landmark is unfixed, add it.
                if (fixedLandmarks->count(landmark) == 0) {
                    landmarksToAdd.insert(landmark);
                    for (auto fp : landmark->fps) {
                        auto frame = fp->frame;
                        if (frameSet->count(frame) == 0) continue;

                        framesToAdd.insert(frame);
                        fpToAdd.insert(make_pair(fp, landmark));
                    }
                } else {//Landmark is fixed. So it needs to be added only if 
                        //there are unfixed frames attached to it
                    for (auto fp : landmark->fps) {
                        auto frame = fp->frame;
                        if (frameSet->count(frame) == 0) continue;
                        //If both landmark and frame is fixed, there
                        //is no point adding the edge
                        if (fixedFrames->count(frame) > 0) continue;

                        //Adding the landmark within the for loop, so that
                        //it gets added only if there is at least non-fixed frame
                        landmarksToAdd.insert(landmark);
                        framesToAdd.insert(fp->frame);
                        fpToAdd.insert(make_pair(fp, landmark));
                    }
                }
            }

            // DEBUG_COUT("Frames to add "<<framesToAdd.size()<<endl);
            // for (auto f : framesToAdd) DEBUG_COUT(f->id<<", ");
            // DEBUG_COUT(endl);

            // DEBUG_COUT("FP to add "<<fpToAdd.size()<<endl);
            // for (auto& [fp, l] : fpToAdd) DEBUG_COUT(fp->id<<": "<<fp->frame->id<<": "<<l->id<<", ");
            // DEBUG_COUT(endl);

            // DEBUG_COUT("Landmark to add "<<landmarksToAdd.size()<<endl);
            // for (auto l : landmarksToAdd) DEBUG_COUT(l->id<<", ");
            // DEBUG_COUT(endl);

            auto [frameRank, maxRank] = generate_frame_rank(
                landmarkSet, 
                frameSet, 
                fixedLandmarks, 
                fixedFrames, 
                landmarkSet->size()>10?10:landmarkSet->size());


            for (auto frame : framesToAdd) {
                auto pose = frame->pose;
                if (framePoseMap && framePoseMap->count(frame) != 0) {
                    pose = (*framePoseMap)[frame];
                }
                // DEBUG_COUT("Adding Frame "<<frame->id<<endl);
                ba->addPose(frame, pose, fixedFrames->count(frame) > 0);
            }
            for (auto landmark : landmarksToAdd) {
                Vector3d& landmarkPos = landmark->trans;
                if (landmarkPos[0] == 0 && landmarkPos[1] == 0 && landmarkPos[2] == 0) {
                    DEBUG_COUT("Landmark Pos1 "<<landmarkPos[0]<<", ");
                    DEBUG_COUT(landmarkPos[1]<<", "<<landmarkPos[2]<<endl);
                }
                if (landmarkTransMap && landmarkTransMap->count(landmark) != 0) {
                    landmarkPos = (*landmarkTransMap)[landmark];
                    if (landmarkPos[0] == 0 && landmarkPos[1] == 0 && landmarkPos[2] == 0) {
                        DEBUG_COUT("Landmark Pos2 "<<landmarkPos[0]<<", ");
                        DEBUG_COUT(landmarkPos[1]<<", "<<landmarkPos[2]<<endl);
                    }
                }
                // DEBUG_COUT("Adding Landmark "<<landmark->id<<", "<<landmarkPos[2]<<endl);
                ba->addLandmark(landmark, landmarkPos, fixedLandmarks->count(landmark) > 0);
            }
            for (auto const& fpPair : fpToAdd) {
                // DEBUG_COUT("Adding Edge "<<fpPair.first<<", "<<fpPair.second<<endl);
                ba->addFramepoint(fpPair.first, fpPair.second, _slamCfg.normalizeKP,
                        _cameraMatrix, _distCoeffs, 
                        (maxRank - (*frameRank)[fpPair.first->frame])*100);
            }

            return make_tuple(frameRank, maxRank);
        }

    public:
        BaHelper(SlamConfig& slamCfg, 
            SP<Frame> currFrame, 
            SP<FrameManager> fm, 
            SP<LandmarkManager> lm,
            const Mat& cameraMatrix, 
            const Mat& distCoeffs
        ) : _slamCfg(slamCfg), 
            _currFrame(currFrame), 
            _estimateValidator(make_shared<EstimateValidator>(
                _slamCfg,
                lm, 
                fm,
                cameraMatrix, 
                distCoeffs)
            ), 
            _fm(fm), 
            _lm(lm), 
            _cameraMatrix(cameraMatrix), 
            _distCoeffs(distCoeffs) 
        {}


        /**
         * @brief Generates an estimate of unfixed landmarks and frames using BA
         * 
         * @param landmarkSet 
         * @param frameSetArg 
         * @param fixedLandmarks 
         * @param fixedFrames 
         * @param iterations 
         * @param inlierRange 
         * @param goodLandmarkRatio 
         * @param goodFrameRatio 
         * @param goodAvgInlierRatio 
         * @param validate 
         * @param landmarkTransMap 
         * @param framePoseMap 
         * @return SP<BaHelperOutput> 
         */
        SP<BaHelperOutput> estimate(SP<LandmarkSet> landmarkSet, 
                SP<FrameSet> frameSetArg,
                SP<LandmarkSet> fixedLandmarks,
                SP<FrameSet> fixedFrames,
                int iterations,
                float inlierRange,
                float goodLandmarkRatio,
                float goodFrameRatio,
                float goodAvgInlierRatio,
                bool validate = true,
                SP<LandmarkTransMap> landmarkTransMap = nullptr,
                SP<FramePoseMap> framePoseMap = nullptr) 
        {
            if (fixedLandmarks == nullptr) fixedLandmarks = make_shared<LandmarkSet>();
            if (fixedFrames == nullptr) fixedFrames = make_shared<FrameSet>();

            auto startTime = Timer::time();

            //Clean up frameSet
            auto frameSet = make_shared<FrameSet>();
            for (auto l : *landmarkSet) {
                for (auto fp : l->fps) {
                    if (frameSetArg->count(fp->frame)) 
                        frameSet->insert(fp->frame);
                }
            }
            
            if (!landmarkTransMap && _output) {
                landmarkTransMap = _output->validatorOutput->landmarkTransMap;
            }
            if (!framePoseMap && _output) {
                framePoseMap = _output->validatorOutput->framePoseMap;
            }
            
            SP<BaHelperOutput> bestOutput;
            
            auto initTime = Timer::diff(startTime);
            startTime = Timer::time();

            _ba = generate_ba(iterations);
            
            auto [frameRank, maxRank] = configure_ba_graph(_ba, 
                landmarkSet, 
                frameSet,
                fixedLandmarks, 
                fixedFrames, 
                landmarkTransMap, 
                framePoseMap);

            auto graphTime = Timer::diff(startTime);
            startTime = Timer::time();
            _ba->optimize(iterations);

            auto optimizeTime = Timer::diff(startTime);
            startTime = Timer::time();

            auto [landmarkTransMap2, framePoseMap2] = _ba->get_estimates();
            if (landmarkTransMap) {
                for (auto& [l, trans] : *landmarkTransMap) {
                    if (landmarkTransMap2->count(l) == 0)
                        (*landmarkTransMap2)[l] = trans;
                }
            }
            if (framePoseMap) {
                for (auto& [f, pose] : *framePoseMap) {
                    if (framePoseMap2->count(f) == 0)
                        (*framePoseMap2)[f] = make_shared<Pose>(pose);
                }
            }

            auto validatorOutput = _estimateValidator->validate_estimates(
                        _ba, 
                        landmarkSet, 
                        frameSet,
                        fixedLandmarks, 
                        fixedFrames,
                        frameRank, 
                        maxRank, 
                        inlierRange,
                        goodLandmarkRatio,
                        goodFrameRatio,
                        goodAvgInlierRatio,
                        landmarkTransMap2, 
                        framePoseMap2,
                        validate);

            _output = make_shared<BaHelperOutput>(
                    landmarkSet, 
                    frameSet,
                    fixedLandmarks, 
                    fixedFrames,
                    frameRank, 
                    maxRank,
                    validatorOutput);
            auto validateTime = Timer::diff(startTime);
            DEBUG_COUT(_currFrame->id<<": BAHelper Time Init "<<initTime<<" Graph "<<graphTime);
            DEBUG_COUT(" Optimize "<<optimizeTime<<" Validate "<<validateTime);
            DEBUG_COUT(endl);

            // DEBUG_COUT(currFrame->id<<": Validity check final Lcnt: "<<output->validLandmarkCnt);
            // DEBUG_COUT(", FpLcnt: "<<output->validFpLandmarkCnt);
            // DEBUG_COUT(", Fcnt: "<<output->validFrameCnt<<endl);

            return _output;
        }

        SP<BaHelperOutput> iterate(
            int iterations,
            float inlierRange,
            float goodLandmarkRatio,
            float goodFrameRatio,
            float goodAvgInlierRatio) 
        {
            auto startTimer = Timer::time();
            _ba->optimize(iterations);
                
            DEBUG_COUT("BA iterate Final Time "<<Timer::diff(startTimer)<<endl);

            startTimer = Timer::time();
            auto [landmarkTransMap, framePoseMap] = _ba->get_estimates();

            auto validatorOutput = _estimateValidator->validate_estimates(
                        _ba, 
                        _output->landmarkSet, 
                        _output->frameSet,
                        _output->fixedLandmarks, 
                        _output->fixedFrames,
                        _output->frameRank, 
                        _output->maxRank,
                        inlierRange,
                        goodLandmarkRatio,
                        goodFrameRatio,
                        goodAvgInlierRatio,
                        landmarkTransMap, 
                        framePoseMap);
            _output->validatorOutput = validatorOutput;

            DEBUG_COUT("BA Validation Time "<<Timer::diff(startTimer)<<endl);
            // DEBUG_COUT(currFrame->id<<": Validity check final Lcnt: "<<output->validLandmarkCnt);
            // DEBUG_COUT(", FpLcnt: "<<output->validFpLandmarkCnt);
            // DEBUG_COUT(", Fcnt: "<<output->validFrameCnt<<endl);
            return _output;
        }

        SP<BaHelperOutput> get_best(SP<BaHelperOutput> output1, SP<BaHelperOutput> output2) {
            if (!output1) return output2;
            else if (!output2) return output1;
            auto vo1 = output1->validatorOutput;
            auto vo2 = output2->validatorOutput;
            if (vo1->validFrameRatio < vo2->validFrameRatio) return output2;
            else if (vo1->validFrameRatio > vo2->validFrameRatio) return output1;
            else if (vo1->avgInlierRatio < vo2->avgInlierRatio) return output2;
            else if (vo1->avgInlierRatio > vo2->avgInlierRatio) return output1;
            else if (vo1->landmarkResult->size(VALID) < vo2->landmarkResult->size(VALID)) return output2;
            else if (vo1->landmarkResult->size(VALID) > vo2->landmarkResult->size(VALID)) return output1;
            else return output1;
        }
        

        /**
         * @brief Copies estimates back to frames and landmarks
         * 
         * @param updateKeyFrameLandmarks Update keyframes too
         * @param scale Optional scale parameter
         */
        void copy_estimates(bool deleteBadFps, float scale = 1) {
            if (!_output) return;
            auto vo = _output->validatorOutput;

            auto frameSet = _output->frameSet;
            for (auto const& [landmark, trans] : *vo->landmarkTransMap) {
                if (_output->landmarkSet->count(landmark) == 0) continue;
                auto landmarkResult = vo->landmarkResult->get(landmark);
                if (deleteBadFps) {
                    if (landmarkResult == VALID) {
                        FramePointSet deleteFps;
                        for (auto fp : landmark->fps) {
                            if (frameSet->count(fp->frame) == 0) continue;
                            if (vo->fpLandmarkResult->count(fp) == 0 ||
                                (*vo->fpLandmarkResult)[fp].count(landmark) == 0) {
                                DEBUG_COUT("FP is missing in fpLandmarkResult "<<fp->id);
                                DEBUG_COUT(", Frame "<<fp->frame->id<<", L "<<landmark->id);
                                if (!fp->landmark.expired()) {
                                    DEBUG_COUT(", Orig Landmark "<<fp->landmark.lock()->id);
                                } else {
                                    DEBUG_COUT(", Orig Landmark "<<-1);
                                }
                                DEBUG_COUT(", landmarkResult "<<vo->landmarkResult->exists(landmark));
                                DEBUG_COUT(", frameResult "<<vo->frameResult->exists(fp->frame));
                                DEBUG_COUT(endl);
                            }
                            // assert(vo->fpLandmarkResult->count(fp) > 0);
                            // assert((*vo->fpLandmarkResult)[fp].count(landmark) > 0);
                            if (vo->fpLandmarkResult->count(fp) == 0 || \
                                (*vo->fpLandmarkResult)[fp].count(landmark) == 0) continue;
                            auto fpResult = (*vo->fpLandmarkResult)[fp][landmark]->result;
                            if (fpResult == FIXED) continue;
                            if (fpResult != VALID || (landmarkResult != VALID && landmarkResult != FIXED)) 
                                deleteFps.insert(fp);
                        }
                        for (auto fp : deleteFps) {
                            _lm->remove_point_from_landmark(landmark, fp);
                        }
                    }
                }
                if (landmarkResult == VALID) {
                    landmark->trans[0] = trans[0] * scale;
                    landmark->trans[1] = trans[1] * scale;
                    landmark->trans[2] = trans[2] * scale;
                    landmark->valid = true;
                }
            }
            
            for (auto const& [frame, pose]: *vo->framePoseMap) {
                if (_output->frameSet->count(frame) == 0) continue;
                auto frameResult = vo->frameResult->get(frame);
                if (frameResult == VALID) {
                    frame->pose->trans[0] = pose->trans[0] * scale;
                    frame->pose->trans[1] = pose->trans[1] * scale;
                    frame->pose->trans[2] = pose->trans[2] * scale;
                    if (_slamCfg.copyRotation) frame->pose->rot = pose->rot;
                }
            }
        }

        SP<BaHelperOutput> get_output() {
            return _output;
        }
        
        double get_error() {
            if (!_output) return -1;
            auto vo = _output->validatorOutput;
            auto landmarkSet = _output->landmarkSet;
            auto frameSet = _output->frameSet;
            double error = 0;
            for (auto landmark : *landmarkSet) {
                auto landmarkTransMap = vo->landmarkTransMap;
                if (landmarkTransMap->count(landmark) == 0) continue;
                auto& landmarkPos = (*landmarkTransMap)[landmark];
                auto framePoseMap = vo->framePoseMap;
                for (auto fp : landmark->fps) {
                    if (frameSet->count(fp->frame) == 0) continue;
                    auto framePose = (*framePoseMap)[fp->frame];
                    auto [px, py] = _ba->getProjection(framePose, landmarkPos);
                    error += TransformUtils::gap(fp, px, py, 
                        _slamCfg.normalizeKP, _cameraMatrix, _distCoeffs);
                }
            }
            return error;
        }
        
        static void replace_landmark(SP<BaHelperOutput> output, SP<Landmark> orig, SP<Landmark> landmark) {
            if (orig == landmark) return;

            if (output->fixedLandmarks->count(orig)) {
                output->fixedLandmarks->erase(orig);
                output->fixedLandmarks->insert(landmark);
            }

            if (output->landmarkSet->count(orig)) {
                output->landmarkSet->erase(orig);
                output->landmarkSet->insert(landmark);
                EstimateValidator::replace_landmark(output->validatorOutput, orig, landmark);
            }
        }
};

#endif /* __BA_HELPER_HPP__ */