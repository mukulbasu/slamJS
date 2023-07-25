/**
 * @file matcher.hpp
 * @brief Matches framepoints between frames.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __MATCHER_HPP__
#define __MATCHER_HPP__

#include <iostream>
#include <vector>
#include <map>
#include "landmarkManager.hpp"
#include "frameManager.hpp"
#include "../utils/transformUtils.hpp"

using namespace std;
using namespace Eigen;
using namespace g2o;

class Matcher {
    protected:
        SlamConfig& _cfg;
        SP<LandmarkManager> _lm;
        SP<FrameManager> _fm;
        g2o::CameraParameters* _camera;
        Vector3d _origin = Vector3d(0, 0, 0);
        SP<LandmarkSet> emptyLandmarkSet = make_shared<LandmarkSet>();

        void set_distances(
            const double distance, 
            double& distance1, 
            double& distance2) 
        {
            if (distance1 == -1) {
                distance1 = distance;
            } else if (distance < distance1) {
                distance2 = distance1;
                distance1 = distance;
            } else if (distance2 == -1 || distance < distance2) {
                distance2 = distance;
            }
        }

        tuple<float, float> get_rotated_projection(
            const Quaterniond& rotDiff, 
            const float matchX, 
            const float matchY) 
        {
            auto newKp = TransformUtils::get_projection(
                    _camera, rotDiff, _origin, 
                    Vector3d(matchX*100/_cfg.fx, matchY*100/_cfg.fx, 100));
            return make_tuple(newKp[0], newKp[1]);
        }

        bool valid_distance(
            double distance1, 
            double distance2) 
        {
            if (distance1 >= _cfg.distanceThreshold) {
                return false;
            } 
            // Check distance2 is much higher than distance1 to eliminate false matches
            else if (distance2 != -1 && distance1 != -1 && distance1 > _cfg.ratio*distance2) {
                return false;
            }
            return true;
        }

        tuple<SP<FramePoint>, double, double> match_nodes(
            SP<FramePoint> currFp, 
            MatchNodeVec& matchNodes, 
            const Quaterniond& rotDiff,
            SP<FrameSet> descriptorFrames) 
        {
            SP<FramePoint> matchFp;
            double minDistance = -1;
            SP<MatchNode> minNode;
            // MatchPriorityQueue que;
            for (auto matchNode : matchNodes) {
                auto prevFp = matchNode->fp;
                auto distance = TransformUtils::get_distance(prevFp, currFp, descriptorFrames);
                // que.push_back(make_pair(matchNode, distance));
                if (minDistance == -1 || minDistance > distance) {
                    minDistance = distance;
                    minNode = matchNode;
                }
            }
            // std::sort(que.begin(), que.end(), [](MatchNodeDist& a, MatchNodeDist& b) {
            //     return a.second > b.second;
            // });
            // while(!matchFp && que.size()>0) {
                // auto lastEle = que[que.size() - 1];
                // minNode = lastEle.first;
                // minDistance = lastEle.second;
                // que.pop_back();
                auto [valid, gap] = valid_gap(
                    currFp->x, 
                    currFp->y, 
                    minNode->fp->x, 
                    minNode->fp->y, 
                    rotDiff);
                if ( valid && minDistance <= _cfg.distanceThreshold) {
                    matchFp = minNode->fp;
                    // cout<<currFp->id<<" Match Found ";
                    // cout<<minNode->fp->id<<" dist "<<minDistance<<endl;
                } else if (minNode->children.size() > 0) {
                    tie(matchFp, minDistance, gap) = match_nodes(
                            currFp, minNode->children, rotDiff, descriptorFrames);
                }
            // }
            return make_tuple(matchFp, minDistance, gap);
        }

    public:
        Matcher(SlamConfig& cfg, SP<LandmarkManager> lm, 
        SP<FrameManager> fm) :
            _cfg(cfg), _lm(lm), _fm(fm), 
            _camera(new CameraParameters(_cfg.fx, g2o::Vector2(0, 0), 0)) {
            cout<<"MaxGap matchers cpp "<<_cfg.maxGap<<endl;
        }

        tuple<bool, double> valid_gap(const float refX, const float refY, const float matchXOrig, const float matchYOrig, const Quaterniond& rotDiff) {
            float matchX = matchXOrig;
            float matchY = matchYOrig;
            tie(matchX, matchY) = get_rotated_projection(rotDiff, matchX, matchY);
            
            auto gap = cv::norm(Point2f(refX - matchX, refY - matchY));
            if (gap > _cfg.maxGap || gap < _cfg.minGap) {
                return make_tuple(false, gap);
            }
            return make_tuple(true, gap);
        }

        SP<LandmarkSet> match_fps(
            SP<Frame> currFrame,  
            FramePointSet& prevFps, 
            SP<FrameSet> descriptorFrames,
            int maxMatches,
            double minAvgGap) 
        {
            if (prevFps.size() == 0) return emptyLandmarkSet;
            SP<Frame> prevFrame = (*prevFps.begin())->frame;
            auto landmarkSet = make_shared<LandmarkSet>();
            LandmarkDistancePairVec ldPairVec;
            auto rotDiff = (currFrame->pose->rot.conjugate() * prevFrame->pose->rot);
            double totalGap = 0;
            int totalPts = 0;

            FramePointSet fpsPending;
            fpsPending.insert(prevFps.begin(), prevFps.end());

            while(fpsPending.size() > 0 && (int)landmarkSet->size() < maxMatches) {
                auto prevFp = TransformUtils::pop_random<SP<FramePoint>>(fpsPending);
                SP<FramePoint> matchFp;
                double distance1 = -1;
                double distance2 = -1;
                double matchGap = 0;

                if (_cfg.matchHierarchy) {
                    for (auto matchNodes : currFrame->matchTree) {
                        auto [fp, distance, gap] = match_nodes(prevFp, matchNodes, rotDiff, descriptorFrames);
                        set_distances(distance, distance1, distance2);
                        if (distance1 == distance) {
                            matchFp = fp;
                            matchGap = gap;
                        }
                    }
                } else {
                    for (auto currFp : currFrame->fps) {
                        auto [valid, gap] = valid_gap(prevFp->x, prevFp->y, currFp->x, currFp->y, rotDiff);
                        if (!valid) continue;
                        auto distance = TransformUtils::get_distance(currFp, prevFp, descriptorFrames);
                        set_distances(distance, distance1, distance2);
                        if (distance1 == distance) {
                            matchFp = currFp;
                            matchGap = gap;
                        }
                    }
                }

                if (matchFp) {
                    if (valid_distance(distance1, distance2)) {
                        // cv::Point2f matchPt = matchFp->kp.pt;
                        // cout<<currFrame->id<<": Matcher "<<currFp->id<<" vs ";
                        // cout<<matchFp->id<<"_"<<prevFrame->id<<":";
                        // cout<<currFp->kp.pt.x<<", "<<currFp->kp.pt.y<<" vs ";
                        // cout<<matchPt.x<<", "<<matchPt.y<<" Summary ";
                        // cout<<cv::norm(matchPt-currFp->kp.pt)<<endl;
                        SP<Landmark> landmark;
                        landmark = _lm->create_floating_landmark_fp(prevFp, matchFp);
                        landmarkSet->insert(landmark);
                        if (landmark->fps.size() == 1) {
                            cout<<"Match Frames Landmark "<<landmark->id<<" size "<<landmark->fps.size()<<endl;
                            assert(false);
                        }
                        totalGap += matchGap;
                        totalPts += 1;
                    }
                }
            }

            if (totalGap/totalPts >= minAvgGap) {
                return landmarkSet;
            } else {
                return emptyLandmarkSet;
            }
        }
};


#endif /* __MATCHER_HPP__*/