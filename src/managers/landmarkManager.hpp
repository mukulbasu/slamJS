/**
 * @file landmarkManager.hpp
 * @brief Manages the set of landmarks that have been detected.
 * A landmark is a 3d point that has been noticed as a keypoint in more than 1
 * camera frame. It primarily deals with 2 types of landmarks:
 * 1. floating landmarks: These are created temporarily for the purpose of extracting valid matches. 
 * These are not tracked and supposed to be discarded or replaced with non-floating landmarks, 
 * if they have been judged to be valid match.
 * 2. non-floating landmarks or just landmarks: These are created and tracked by landmark manager. 
 * These are not deleted unless explicitly done so.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __LANDMARK_MANAGER_HPP__
#define __LANDMARK_MANAGER_HPP__

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <assert.h>
#include "../types/types.hpp"

using namespace std;
using namespace Eigen;

class LandmarkManager {
protected:
    SlamConfig& _cfg;
    SP<LandmarkSet> _landmarks = make_shared<LandmarkSet>();
    int idCnt = 989900000;

public:

protected:
    void set_initial_estimate(SP<FramePoint> fp, Vector3d& trans) {
        trans = Vector3d{fp->x * _cfg.maxDepth/_cfg.fx, \
                    fp->y*_cfg.maxDepth/_cfg.fy, (double)_cfg.maxDepth};
        trans = fp->frame->pose->trans + fp->frame->pose->rot * trans;
    }

public:
    LandmarkManager(SlamConfig& cfg) : 
    _cfg(cfg) {}

    SP<Landmark> create_landmark(SP<FramePoint> fp, double distance) {
        auto landmark = make_shared<Landmark>();
        landmark->id = idCnt;
        idCnt++;
        _landmarks->insert(landmark);
        fp->landmark = landmark;
        fp->matchDistance = distance;
        landmark->fps.insert(fp);

        set_initial_estimate(fp, landmark->trans);

        // DEBUG_COUT(fp->frameId<<":"<<fp->id);
        // DEBUG_COUT(" Create Landmark L "<<landmark->id<<endl);

        return landmark;
    }

    SP<Landmark> create_floating_landmark_l(SP<Landmark> srcLandmark, SP<FramePoint> fp = nullptr) {
        SP<Landmark> landmark = make_shared<Landmark>();
        landmark->trans[0] = srcLandmark->trans[0];
        landmark->trans[1] = srcLandmark->trans[1];
        landmark->trans[2] = srcLandmark->trans[2];
        // landmark->fixed = srcLandmark->fixed;
        for (auto ele : srcLandmark->fps) landmark->fps.insert(ele);
        landmark->id = srcLandmark->id;
        // landmark->srcLandmark = srcLandmark;

        if (fp) {
            landmark->fps.insert(fp);
            dedupe_landmark_points(landmark);
        }

        return landmark;
    }

    SP<Landmark> create_floating_landmark_fp(SP<FramePoint> fp1, SP<FramePoint> fp2) {
        assert(fp1 != fp2);
        SP<Landmark> landmark = make_shared<Landmark>();
        
        auto landmarkFp1 = fp1->landmark.lock();
        auto landmarkFp2 = fp1->landmark.lock();
        if (landmarkFp1 && landmarkFp2 && landmarkFp1->valid && landmarkFp2->valid) {
            for (int i = 0; i<3; i++) {
                landmark->trans[i] = (landmarkFp1->trans[i] + landmarkFp2->trans[i])/2;
            }
        } else if (landmarkFp1 && landmarkFp1->valid) {
            for (int i = 0; i<3; i++) landmark->trans[i] = landmarkFp1->trans[i];
        } else if (landmarkFp2 && landmarkFp2->valid) {
            for (int i = 0; i<3; i++) landmark->trans[i] = landmarkFp2->trans[i];
        } else {
            //This is crucial. Otherwise Bundle Adjustment will fail
            set_initial_estimate(fp1, landmark->trans);
        }
        
        landmark->fps.insert(fp1);
        landmark->fps.insert(fp2);
        landmark->id = idCnt;
        idCnt++;

        return landmark;
    }

    void remove_landmark(SP<Landmark> landmark) {
        for (auto fp : landmark->fps) {
            if (landmark == fp->landmark.lock()) {
                fp->landmark.reset();
                fp->matchDistance = INITIAL_DISTANCE;
            }
        }
        _landmarks->erase(landmark);
    }

    //Return value is true if landmark itself got deleted.
    bool remove_point_from_landmark(SP<Landmark> landmark, SP<FramePoint> fp) {
        landmark->fps.erase(fp);
        //FP can be associated with multiple temporary floating landmarks too.
        //So before resetting the FP landmark, check if that is the landmark being modified
        if (landmark == fp->landmark.lock()) {
            fp->landmark.reset();
            fp->matchDistance = INITIAL_DISTANCE;
        }
        if (landmark->fps.size() < 2) {
            remove_landmark(landmark);
            return true;
        }
        return false;
    }

    //Return value indicates if landmark itself got deleted
    bool remove_point_from_landmark(SP<Landmark> landmark, int frameId) {
        for (auto fp : landmark->fps) {
            if (frameId == fp->frame->id) {
                return remove_point_from_landmark(landmark, fp);
            }
        }
        return false;
    }

    void merge_landmarks(SP<Landmark> ref, SP<Landmark> merge) {
        if (ref->id != merge->id) {
            //Copy over the points from second landmark to first
            for (auto fp : merge->fps) {
                ref->fps.insert(fp);
                fp->landmark = ref;
            }

            //Copy the estimates to landmark1 if landmark2 has a valid estimate
            //Estimate being fixed indicates that atleast 1 valid estimate has been set
            if (merge->valid) {
                if (!ref->valid) {
                    ref->trans = merge->trans;
                    ref->valid = true;
                }
            }
            dedupe_landmark_points(ref);
            _landmarks->erase(merge);
        }
    }

    SP<LandmarkSet> get_landmarks() { return _landmarks; }

    static void dedupe_landmark_points(SP<Landmark> landmark) {
        set<int> frameIds;
        for (auto point : landmark->fps) {
            frameIds.insert(point->frame->id);
        }
        for (auto frameId : frameIds) {
            vector<SP<FramePoint>> duplicates;
            for (auto fp : landmark->fps) {
                if (frameId == fp->frame->id) duplicates.push_back(fp);
            }
            if (duplicates.size() > 1) {
                SP<FramePoint> selectedPoint;
                double minDistance = -1;
                for (auto duplicate : duplicates) {
                    double distance = 0;
                    for (auto point : landmark->fps) {
                        if (duplicate != point) distance += norm(duplicate->desc, point->desc, NORM_HAMMING);
                    }
                    if (-1 == minDistance || distance < minDistance) {
                        minDistance = distance;
                        selectedPoint = duplicate; 
                    }
                }
                for (auto duplicate : duplicates) {
                    if (selectedPoint != duplicate) {
                        // {
                        //     DEBUG_COUT(duplicate->frameId<<":"<<duplicate->id);
                        //     if (duplicate->landmark.expired())
                        //         DEBUG_COUT(" Dedupe Landmark L "<<landmark->id<<endl);
                        //     else 
                        //         DEBUG_COUT(" Dedupe Landmark L "<<landmark->id<<" FP's L "<<duplicate->landmark.lock()->id<<endl);
                        // }
                        duplicate->landmark.reset();
                        duplicate->matchDistance = INITIAL_DISTANCE;
                        landmark->fps.erase(duplicate);
                        DEBUG_COUT(frameId<<":"<<duplicate->id<<":"<<landmark->id<<": Dedupe deleted"<<endl);
                        DEBUG_COUT(frameId<<"Deduped "<<duplicate->id<<" VS "<<selectedPoint->id<<endl);
                    }
                }
            }
        }
    }

    static void link_landmark_point(SP<Landmark> landmark, SP<FramePoint> fp, double distance) {
        // {
        //     DEBUG_COUT(fp->frameId<<":"<<fp->id;
        //     if (fp->landmark.expired())
        //         DEBUG_COUT(" Link FP L "<<landmark->id<<endl;
        //     else 
        //         DEBUG_COUT(" Link FP L "<<landmark->id<<" FP's L "<<fp->landmark.lock()->id<<endl;
        // }
        landmark->fps.insert(fp);
        fp->landmark = landmark;
        fp->matchDistance = distance;
        // DEBUG_COUT("Linking Landmark:"<<landmark->id<<" FrameID:"<<fp->frameId<<" KPID:"<<fp->id<<endl;
        dedupe_landmark_points(landmark);
    }

    static SP<FrameSet> extract_frames(SP<LandmarkSet> landmarkSet) {
        auto frameSet = make_shared<FrameSet>();
        for (auto landmark : *landmarkSet)
            for (auto fp : landmark->fps)
                frameSet->insert(fp->frame);
        return frameSet;
    }

    static SP<FrameLandmarksMap> extract_frame_landmark_map(
                SP<LandmarkSet> landmarkSet) {
        auto frameLandmarksMap = make_shared<FrameLandmarksMap>();
        for (auto landmark : *landmarkSet) {
            for (auto fp : landmark->fps) {
                if (frameLandmarksMap->count(fp->frame) == 0) {
                    (*frameLandmarksMap)[fp->frame] = make_shared<LandmarkSet>();
                }
                (*frameLandmarksMap)[fp->frame]->insert(landmark);
            }
        }
        return frameLandmarksMap;
    }

    static SP<FrameLandmarksPairVec> sort(SP<FrameLandmarksMap> landmarkFramesMap) {
        auto values = make_shared<FrameLandmarksPairVec>(landmarkFramesMap->begin(), landmarkFramesMap->end());
        std::sort(values->begin(), values->end(), [](const FrameLandmarksPair& a, const FrameLandmarksPair& b) {
            return a.second->size() > b.second->size();
        });
        return values;
    }

    static SP<LandmarkFramesMap> extract_landmark_frame_map(
                SP<LandmarkSet> landmarkSet) {
        auto landmarkFramesMap = make_shared<LandmarkFramesMap>();
        for (auto landmark : *landmarkSet) {
            (*landmarkFramesMap)[landmark] = make_shared<FrameSet>();
            for (auto fp : landmark->fps) {
                (*landmarkFramesMap)[landmark]->insert(fp->frame);
            }
        }
        return landmarkFramesMap;
    }

    static SP<LandmarkFramesPairVec> sort(SP<LandmarkFramesMap> landmarkFramesMap) {
        auto values = make_shared<LandmarkFramesPairVec>(landmarkFramesMap->begin(), landmarkFramesMap->end());
        std::sort(values->begin(), values->end(), [](auto const& a, auto const& b) {
            return a.second->size() > b.second->size();
        });
        return values;
    }

    SP<LandmarkSet> clone_landmarks(SP<LandmarkSet> landmarkSet) {
        auto cloneSet = make_shared<LandmarkSet>();
        for (auto landmark : *landmarkSet) {
            SP<Landmark> clone = create_floating_landmark_l(landmark);
            clone->id = landmark->id;
            cloneSet->insert(clone);
        }
        return cloneSet;
    }

    void set_origin_trans(Vector3d& originTrans) {
        for (auto landmark : *_landmarks) {
            landmark->trans = landmark->trans - originTrans;
        }
    }
};


#endif /* __LANDMARK_MANAGER_HPP__*/