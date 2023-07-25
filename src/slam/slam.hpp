/**
 * @file slam.hpp
 * @brief This class is the starting point for SLAM and
 * provides the entry point functions for SLAM. This class primarily 
 * uses orbExtractor to extract keypoints and poseManager for estimating
 * the camera position.
 * The key methods are:
 * 1. extract_keypoints: Used to add a new camera image and extract keypoints
 * 2. process: Process the generated keypoints for pose computation
 * 2. initialize: Used to initialize the SLAM system after a few frames have been added
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __SLAM_HPP__
#define __SLAM_HPP__

// for std
#include <iostream>
#include <map>

#include "../imageAnalysis/orbExtractor.h"
#include "../utils/timer.hpp"
#include "../managers/poseManager.hpp"

using namespace std;
using namespace cv;
using namespace Eigen;

class Slam {
    public:
        SlamConfig cfg;
        SP<FrameManager> fm;
        SP<LandmarkManager> lm;
        bool initialized = false;
    protected:
        SP<Matcher> _matcher;
        SP<PoseManager> _pm;
        Ptr<ORB> _orb;//This is not used, but removing it generates build errors related to cv::FAST
        ORBextractor _orbExtractorInit;
        ORBextractor _orbExtractor;

        void populate_match_node(vector<SP<MatchNodeInt>>& matchNodes, set<int>& kpsPending, SP<Mat> descs,
                    int branchSize, int leafSize) {
            int maxSize = ((int)kpsPending.size() < leafSize)? (int)kpsPending.size() : branchSize;
            for (int index = 0; index < maxSize; index++) {
                auto kpIndex = TransformUtils::pop_random<int>(kpsPending);
                matchNodes.push_back(make_shared<MatchNodeInt>(kpIndex));
            }
            if (kpsPending.size() == 0) return;

            map<SP<MatchNodeInt>, set<int>> nodeKpIndices;
            for (auto kpIndex : kpsPending) {
                float minDistance = -1;
                SP<MatchNodeInt> minNode = matchNodes[0];
                for (auto childNode : matchNodes) {
                    auto distance = norm(descs->row(kpIndex), descs->row(childNode->index), NORM_HAMMING);
                    if (minDistance == -1 || distance < minDistance) {
                        minDistance = distance;
                        minNode = childNode;
                    }
                }
                nodeKpIndices[minNode].insert(kpIndex);
            }
            for (auto& [node, kpIndices]: nodeKpIndices) {
                populate_match_node(node->children, kpIndices, descs, branchSize, leafSize);
            }
        }

        SP<vector<vector<SP<MatchNodeInt>>>> populate_match_tree(SP<Mat> descs, int size) {
            int leafSize = cfg.leafSize;
            int branchSize = cfg.branchSize;
            int treeSize = cfg.treeSize;
            auto matchTree = make_shared<vector<vector<SP<MatchNodeInt>>>>();

            for (int tree = 0; tree < treeSize; tree++) {
                set<int> kpsPending;
                for (int i = 0; i < size; i++) kpsPending.insert(i);
                vector<SP<MatchNodeInt>> matchNodeVec;
                matchTree->push_back(matchNodeVec);
                populate_match_node((*matchTree)[tree], kpsPending, descs, branchSize, leafSize);
            }
            return matchTree;
        }

        void storeDescriptor(float* array, const cv::Mat& descriptor) {
            CV_Assert(descriptor.type() == CV_8UC1);
            CV_Assert(descriptor.rows == 1);
            CV_Assert(descriptor.cols <= 32);

            std::memcpy(array, descriptor.data, descriptor.cols);
        }

        // Recreate cv::Mat descriptor from a 32-bit integer array
        void recreateDescriptor(const float* array, cv::Mat& descriptor) {
            std::memcpy(descriptor.data, array, descriptor.cols);
        }

        tuple<SP<FramePointVec>, SP<vector<vector<SP<MatchNode>>>>> extract(ExportData* data, SP<Frame> frame) {
            auto fpVec = make_shared<FramePointVec>();
            for (int i = 0; i < data->kpSize; i++) {
                KeyPoint kp;
                kp.pt = Point2f(data->x[i], data->y[i]);
                cv::Mat descriptor(1, 32, CV_8UC1);
                recreateDescriptor(data->desc[i], descriptor);
                auto fp = make_shared<FramePoint>(i, 
                    kp, descriptor, frame, cfg.cx, cfg.cy, 1);
                fpVec->push_back(fp);
            }

            auto matchTree = make_shared<vector<vector<SP<MatchNode>>>>();
            for (int tree = 0; tree < data->treeSize; tree++) {
                vector<SP<MatchNode>> matchNodeVec;
                matchTree->push_back(matchNodeVec);
                vector<SP<MatchNode>> queue;
                int dataIndex = 0, nodeCount = 0;
                while(data->trees[tree][dataIndex] != -1) {
                    auto fp = (*fpVec)[data->trees[tree][dataIndex]];
                    (*matchTree)[tree].push_back(make_shared<MatchNode>(fp));
                    queue.push_back((*matchTree)[tree][(*matchTree)[tree].size() - 1]);
                    nodeCount++;
                    dataIndex++;
                }
                dataIndex++;
                while (queue.size() > 0 && nodeCount < data->kpSize) {
                    auto matchNode = queue[queue.size()-1];
                    queue.pop_back();
                    while(data->trees[tree][dataIndex] != -1 && nodeCount < data->kpSize) {
                        auto fp = (*fpVec)[data->trees[tree][dataIndex]];
                        matchNode->children.push_back(make_shared<MatchNode>(fp));
                        queue.push_back(matchNode->children[matchNode->children.size() - 1]);
                        nodeCount++;
                        dataIndex++;
                    }
                    dataIndex++;
                }
            }

            return make_tuple(fpVec, matchTree);
        }

    public:
        Slam(SlamConfig& cfgArg) : 
        cfg(cfgArg),
        fm{make_shared<FrameManager>(cfg)},
        lm{make_shared<LandmarkManager>(cfg)},
        _matcher{make_shared<Matcher>(cfg, lm, fm)},
        _pm{make_shared<PoseManager>(cfg, lm, _matcher, fm)},
        _orb(ORB::create()),
        _orbExtractorInit(cfg.reqdKpsInit, 1.2, NLEVELS, 20, 7),
        _orbExtractor(cfg.reqdKps, 1.2, NLEVELS, 20, 7) {
            cout<<"MaxGap matchers cpp "<<cfg.maxGap<<endl;
        }

        void extract_keypoints (Mat& img, ExportData* data) 
        {
            //Extract keypoints and descriptors
            auto analysisStart = Timer::time();
            auto kps = make_shared<vector<KeyPoint>>();
            auto descs = make_shared<Mat>();
            if (!initialized) {
                _orbExtractorInit(img, cv::Mat(), *kps, *descs);
            } else {
                _orbExtractor(img, cv::Mat(), *kps, *descs);
            }
            data->imgWidth = img.size().width;
            data->imgHeight = img.size().height;

            auto kpTime = Timer::time() - (analysisStart);
            analysisStart = Timer::time();
           
            auto matchTree = populate_match_tree(descs, kps->size());
            auto treeCreateTime = Timer::time() - (analysisStart);
            cout<<"KP Time "<<kpTime<<" Tree time "<<treeCreateTime<<endl;
            
            data->kpSize = (float)kps->size();
            for (int i = 0; i < (int)kps->size(); i++) data->x[i] = (*kps)[i].pt.x;
            for (int i = 0; i < (int)kps->size(); i++) data->y[i] = (*kps)[i].pt.y;
            for (int i = 0; i < (int)kps->size(); i++) {
                storeDescriptor(data->desc[i], descs->row(i));
            }
            data->treeSize = (float)matchTree->size();
            for (int tree = 0; tree < (int)matchTree->size(); tree++) {
                vector<SP<MatchNodeInt>> queue;
                int index = 0;
                for (int i = 0; i < (int)(*matchTree)[tree].size(); i++) {
                    data->trees[tree][index++] = (*matchTree)[tree][i]->index;
                    queue.push_back((*matchTree)[tree][i]);
                }
                data->trees[tree][index++] = -1;
                while(queue.size() > 0) {
                    auto matchNode = queue[queue.size()-1];
                    queue.pop_back();
                    if (matchNode->children.size() > 0) {
                        for (int childIndex = 0; childIndex < (int)matchNode->children.size(); childIndex++) {
                            data->trees[tree][index++] = matchNode->children[childIndex]->index;
                            queue.push_back(matchNode->children[childIndex]);
                        }
                    }
                    data->trees[tree][index++] = -1;
                }
                data->trees[tree][index++] = -2;
            }
        }

        SP<PoseManagerOutput> process(double orientation[3], int id, int64_t timestamp, ExportData* data)
        {
            //Extract keypoints and descriptors
            auto addStart = Timer::time();
            auto analysisStart = Timer::time();

            auto frameList = fm->frameList;
            auto lastFrameId = frameList->size() == 0? 0 : frameList->at(frameList->size() - 1)->id;
            int frameId = id == -1? lastFrameId + 1 : id;
           
            //Create frame 
            // auto frameStart = Timer::time();
            auto [fpVec, matchTree] = extract(data, nullptr);
            auto currFrame = fm->create_frame(
                frameId, data->imgWidth, data->imgHeight, orientation, timestamp, fpVec, matchTree);
            // cout<<currFrame->id<<": Time FrameInsert: "<<Timer::diff(frameStart)<<endl;
            auto frameCreatTime = Timer::time() - (analysisStart);
            analysisStart = Timer::time();

            //Ensure old frames are deleted
            if ((int)(fm->frameList->size()) > cfg.maxFrames) {
                auto deleteFrame = fm->remove_a_frame();
                if (fm->originFrame != deleteFrame &&
                        fm->get_keyframes()->count(deleteFrame) == 0) {
                    for (auto fp : deleteFrame->fps) {
                        auto landmark = fp->landmark.lock();
                        if (landmark) {
                            lm->remove_point_from_landmark(landmark, fp);
                        }
                    }
                }
            }
            auto frameDeleteTime = Timer::diff(analysisStart);
            analysisStart = Timer::time();

            //Compute Pose
            SP<PoseManagerOutput> result;
            int64_t poseTime = 0;
            if (fm->frameList->size() >= 1) {
                
                result = _pm->add_frame(currFrame);

                poseTime = Timer::time()- analysisStart;
                cout<<", FrameCreate "<<frameCreatTime;
                cout<<", FrameDelete "<<frameDeleteTime;
                cout<<", Pose: "<<poseTime;
                cout<<", Add Image "<<Timer::diff(addStart);
                cout<<", Distance threshold "<<currFrame->landmarkDistThreshold;
                cout<<endl;

                if (_pm->is_initialized()) initialized = true;
            }

            result->profile[FRAME_CREATE_TIME] = frameCreatTime;
            result->profile[POSE_TIME] = poseTime;
            result->profile[OVERALL_TIME] = Timer::time() - addStart;
            return result;
        }
};

#endif /* __SLAM_HPP__ */