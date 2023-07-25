/**
 * @file frameManager.hpp
 * @brief Manages the camera frames. Stores information on the keyframes 
 * as well as the last N frames, the origin frame.
 * Key methods:
 * 1. create_frame: Add a camera frame
 * 2. remove_a_frame: Delete frame. Used to keep the list of frames and memory limited.
 * 3. set_origin_frame: Set origin frame
 * 4. add_keyframe: Add an existing frame to the list of keyframes. Keyframes are the ones mainly used for estimating any new frame.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __FRAME_MANAGER_HPP__
#define __FRAME_MANAGER_HPP__

// for std
#include <iostream>
#include <set>
#include "../types/types.hpp"
#include "../utils/transformUtils.hpp"

using namespace std;
using namespace cv;
using namespace Eigen;


#define MAX_DESCRIPTOR_DIST 60
#define MIN_DIST_REQD 2.0

class FrameManager {
    protected:
        SlamConfig& _cfg;
        double radScl = 44.0/(360*7);
        SP<FrameSet> _keyFrames = make_shared<FrameSet>();
        SP<FrameSet> _initialKeyFrames = make_shared<FrameSet>();
        SP<BOWImgDescriptorExtractor> _bowExtractor;
        Eigen::Vector3d _currTransSmooth;
        Eigen::Vector3d _currVelSmooth;

    public:
        SP<FrameVec> frameList = make_shared<FrameVec>();
        SP<Frame> originFrame;
        Quaterniond originRotInverse;
        float imgWidth;
        float imgHeight;
    
    protected:

        Quaterniond get_rotation(double orientation[3]) {
            if (_cfg.disableRotationInput) {
                Quaterniond w;
                w.setIdentity();
                return w;
            } else {
                return AngleAxisd(orientation[1], Vector3d::UnitY())
                    * AngleAxisd(orientation[0], Vector3d::UnitX())
                    * AngleAxisd(orientation[2], Vector3d::UnitZ());
            }
        }

        void populate_match_node(MatchNodeVec& matchNodes, FramePointSet& fpsPending, 
                    int branchSize, int leafSize, SP<Frame> frame) {
            int maxSize = ((int)fpsPending.size() < leafSize)? (int)fpsPending.size() : branchSize;
            for (int index = 0; index < maxSize; index++) {
                auto fp = TransformUtils::pop_random<SP<FramePoint>>(fpsPending);
                matchNodes.push_back(make_shared<MatchNode>(fp));
            }
            if (fpsPending.size() == 0) return;

            map<SP<MatchNode>, FramePointSet> nodeFps;
            auto descriptorFrames = make_shared<FrameSet>();
            descriptorFrames->insert(frame);
            for (auto fp : fpsPending) {
                float minDistance = -1;
                SP<MatchNode> minNode;
                for (auto childNode : matchNodes) {
                    auto distance = TransformUtils::get_distance(fp, childNode->fp, descriptorFrames);
                    if (minDistance == -1 || distance < minDistance) {
                        minDistance = distance;
                        minNode = childNode;
                    }
                }
                nodeFps[minNode].insert(fp);
            }
            for (auto& [node, fps]: nodeFps) {
                populate_match_node(node->children, fps, branchSize, leafSize, frame);
            }
        }

        void populate_match_tree(SP<Frame> frame) {
            int leafSize = _cfg.leafSize;
            int branchSize = _cfg.branchSize;
            int treeSize = _cfg.treeSize;

            for (int tree = 0; tree < treeSize; tree++) {
                FramePointSet fpsPending(frame->fps);
                MatchNodeVec matchNodeVec;
                frame->matchTree.push_back(matchNodeVec);
                populate_match_node(frame->matchTree[tree], fpsPending, branchSize, leafSize, frame);
            }
        }

        void populate_match_tree(SP<Frame> frame, SP<vector<vector<SP<MatchNodeInt>>>> matchTree, SP<FramePointVec> fpVec) {
            for (int tree = 0; tree < (int)matchTree->size(); tree++) {
                MatchNodeVec matchNodeVec;
                frame->matchTree.push_back(matchNodeVec);

                vector<SP<MatchNodeInt>> queueInt;
                vector<SP<MatchNode>> queue;
                for (auto matchNodeInt : (*matchTree)[tree]) {
                    frame->matchTree[tree].push_back(make_shared<MatchNode>((*fpVec)[matchNodeInt->index]));
                    queueInt.push_back(matchNodeInt);
                    queue.push_back(frame->matchTree[tree][frame->matchTree[tree].size() - 1]);
                }
                while(queue.size() > 0) {
                    auto matchNodeInt = queueInt[queueInt.size() - 1];
                    queueInt.pop_back();
                    auto matchNode = queue[queue.size() - 1];
                    queue.pop_back();
                    if (matchNodeInt->children.size() > 0) {
                        for (auto childNodeInt : matchNodeInt->children) {
                            matchNode->children.push_back(make_shared<MatchNode>((*fpVec)[childNodeInt->index]));
                            queueInt.push_back(childNodeInt);
                            queue.push_back(matchNode->children[matchNode->children.size() - 1]);
                        }
                    }
                }
            }
        }

    public:

        FrameManager(SlamConfig& cfg): _cfg(cfg) {
            _currTransSmooth[0] = 0.0;
            _currTransSmooth[1] = 0.0;
            _currTransSmooth[2] = 0.0;
        }

        SP<Frame> get_current() {
            auto size = frameList->size();
            if (size == 0) return nullptr;
            else return (*frameList)[size - 1];
        }

        /**
         * @brief Add a new camera frame
         * 
         * @param idArg 
         * @param kps 
         * @param descs 
         * @param img 
         * @param orientation 
         * @return SP<Frame> 
         */
        SP<Frame> create_frame(int idArg, 
                float imgWidthArg, 
                float imgHeightArg, 
                double orientation[3],
                int64_t timestamp,
                SP<FramePointVec> fpVec = nullptr,
                SP<vector<vector<SP<MatchNode>>>> matchTree = nullptr) {
            Vector3d trans = Vector3d(0, 0, 0);
            int invalidCount = 0;
            for (int i = 0; i < 3 && i < (int)frameList->size(); i++) {
                if(!(*frameList)[frameList->size() - 1 - i]->valid) invalidCount++;
                else break;
            }
            if (frameList->size() > 0 && invalidCount < 3) {
                trans = get_current()->pose->trans;
                if (get_current()->id >= 2 && get_current()->id < 5) {
                    trans = Vector3d(1, 0, 0);
                } 
            }
            auto rot = get_rotation(orientation);
            auto frame = make_shared<Frame>(idArg, trans, rot, orientation, timestamp);
            if (frameList->size() == 0) {
                imgWidth = imgWidthArg;
                imgHeight = imgHeightArg;
            }

            // frame->descs = descs;
            // if (!fpVec) {
            //     for (int i = 0; i < (int)kps->size(); i++) {
            //         auto desc = descs->row(i);
            //         SP<FramePoint> fp = make_shared<FramePoint>(frame->fps.size(), 
            //                 kps->at(i), desc, frame, _cfg.cx, _cfg.cy, 1);
            //         frame->fps.insert(fp);
            //     }
            //     populate_match_tree(frame);
            // } else {
                for (auto fp : *fpVec) {
                    frame->fps.insert(fp);
                    fp->frame = frame;
                }
                for (int i = 0; i < (int) matchTree->size(); i++) {
                    frame->matchTree.push_back((*matchTree)[i]);
                }
            // }

            if (frameList->size() > 0) {
                (*frameList)[frameList->size() - 1]->isCurrFrame = false;
            }
            frameList->push_back(frame);
            frame->isCurrFrame = true;

            return frame;
        }

        /**
         * @brief Delete a camera frame
         * 
         * @return SP<Frame> 
         */
        SP<Frame> remove_a_frame() {
            auto frame = frameList->at(0);

            for (auto it = frameList->begin(); it != frameList->end(); ++it) {
                if (*it == frame) {
                    frameList->erase(it);
                    break;
                }
            }

            return frame;
        }

        // static SP<FramePoint> pop_random_fp(FramePointSet& fps) {
        //     if (fps.size() == 0) return nullptr;

        //     auto it = std::begin(fps);
        //     std::advance(it, rand() % fps.size());
        //     auto fp = *it;
        //     fps.erase(fp);
        //     return fp;
        // }

        Vector3d set_origin_frame(SP<Frame> frame) {
            originFrame = frame;
            auto originTrans(frame->pose->trans);
            for (auto frameTemp : *frameList) {
                frameTemp->pose->trans = frameTemp->pose->trans - originTrans;
            }
            originRotInverse = frame->pose->rot.conjugate();
            return originTrans;
        }

        /**
         * @brief Add an existing frame to keyframes. Keyframes are not deleted
         * and are used for estimating new frames.
         * 
         * @param frame 
         */
        void add_keyframe(SP<Frame> frame) {
            _keyFrames->insert(frame);
            //Check if first frame
            if (_keyFrames->size() == 1) {
                frame->level = 0;
            } else {
                //calculate the least level keyframe present in current frame's matches.
                //current frames level will be that keyframe's level + 1
                map<int, int> levelVsMatchCount;
                for (auto fp : frame->fps) {
                    if (fp->landmark.expired()) continue;
                    auto landmark = fp->landmark.lock();
                    for (auto otherFp : landmark->fps) {
                        if (otherFp == fp) continue;
                        if (_keyFrames->count(otherFp->frame) > 0) {
                            auto level = otherFp->frame->level;
                            if (levelVsMatchCount.count(level) == 0) levelVsMatchCount[level] = 1;
                            else levelVsMatchCount[level] = levelVsMatchCount[level] + 1;
                        }
                    }
                }
                //Find min level with at least 4 match counts
                int minLevel = -1;
                for (auto [level, matchCount] : levelVsMatchCount) {
                    if (matchCount < 4) continue;
                    if (-1 == minLevel) minLevel = level;
                    else if (minLevel > level) minLevel = level;
                }
                frame->level = minLevel + 1;
            }
            frame->isKeyFrame = true;
        }

        void set_initial_keyframes() {
            _initialKeyFrames = make_shared<FrameSet>();
            _initialKeyFrames->insert(_keyFrames->begin(), _keyFrames->end());
        }

        SP<FrameSet> get_keyframes() { return _keyFrames;}

        SP<FrameSet> get_initial_keyframes() { return _initialKeyFrames;}

        bool check_current_or_keyframe(SP<Frame> frame) {
            return (frameList->at(frameList->size() - 1) == frame) || _keyFrames->count(frame) > 0;
        }

        void populate_frame_landmark_dist_threshold(SP<FrameSet> frameSet) {
            for (auto frame : *frameSet) {
                vector<double> distances;
                for (auto fp : frame->fps) {
                    auto landmark = fp->landmark.lock();
                    if (!landmark || !landmark->valid) continue;
                    distances.push_back((frame->pose->trans - landmark->trans).norm());
                }
                std::sort(distances.begin(), distances.end(), [](const auto& a, const auto& b) {
                    return a < b;
                });
                if (distances.size() > 0) {
                    int indexToPick = 0.5*distances.size();
                    frame->landmarkDistThreshold = distances[indexToPick];
                }
            }
        }

        void set_curr_trans_smoothed(SP<Frame> currFrame) {
            int64_t prevTimestamp = 0;
            for (int i = 2;;i++) {
                if ((int)frameList->size() < i) break;
                auto frame = (*frameList)[frameList->size() - i];
                if (frame->valid) {
                    prevTimestamp = frame->timestamp; 
                    break;
                }
            }
            auto prevTrans = _currTransSmooth;
            auto frameDistance = (currFrame->pose->trans - _currTransSmooth).norm();
            cout<<"Movement comparison "<<frameDistance<<", ";
            cout<<currFrame->landmarkDistThreshold<<", ratio ";
            cout<<((float)frameDistance)/currFrame->landmarkDistThreshold<<endl;
            if (frameDistance <= _cfg.smootheningTolerance * currFrame->landmarkDistThreshold) {
                _currTransSmooth = prevTrans;
            } else {
                _currTransSmooth = (currFrame->pose->trans + prevTrans) / 2 ;
            }
            _currVelSmooth = (_currTransSmooth - prevTrans) / (currFrame->timestamp - prevTimestamp);
        }

        Vector3d get_curr_trans_smoothed() {
            return _currTransSmooth;
        }

        Vector3d get_curr_vel_smoothed() {
            return _currVelSmooth;
        }
};

#endif /* __FRAME_MANAGER_HPP__*/