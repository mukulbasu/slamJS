/**
 * @file transformUtils.hpp
 * @brief Helper class to help with geometrical operations
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __TRANSFORM_UTILS_HPP__
#define __TRANSFORM_UTILS_HPP__

// for std
#include <iostream>
#include <map>
#include <cmath>
// for opencv 
#include "opencv2/calib3d.hpp"
// for g2o
#include "g2o/types/sba/parameter_cameraparameters.h"
#include "g2o/types/slam3d/se3quat.h"
#include "../types/types.hpp"


using namespace std;
using namespace Eigen;

class TransformUtils {

    public:

        static void degToQuaterniond(const double deg[3], Eigen::Quaterniond& rot) {
            double radScl = 44.0/(360*7);
            Eigen::AngleAxisd roll(radScl * deg[0], Eigen::Vector3d::UnitX()); 
            Eigen::AngleAxisd pitch(radScl * deg[1], Eigen::Vector3d::UnitY());
            Eigen::AngleAxisd yaw(radScl * deg[2], Eigen::Vector3d::UnitZ());
            rot = pitch * roll * yaw;
        }

        static double trim(double val) {
            return (double)((int)(val * 1000 + 0.5))/1000;
        }

        static void quaterniondToDeg(const Eigen::Quaterniond& rot, double deg[3]) {
            Eigen::Matrix3d rotMat = rot.toRotationMatrix();
            auto const& euler = rotMat.eulerAngles(1, 0, 2);
            double degScl = 360.0*7/44;
            deg[0] = trim(degScl*euler[0]);
            deg[1] = trim(degScl*euler[1]);
            deg[2] = trim(degScl*euler[2]);
        }
        
        static double get_distance(SP<Landmark> landmark, const Mat& desc, SP<Frame> descFrame, SP<FrameSet> descriptorFrames) {
            double minDistance = INITIAL_DISTANCE;
            // cout<<"Get distance "<<landmark->id<<endl;
            double minFrameDist = -1;
            SP<FramePoint> minFp;
            for (auto fp : landmark->fps) {
                if (descriptorFrames->count(fp->frame) == 0) continue;
                auto frameDist = (fp->frame->pose->trans - descFrame->pose->trans).norm();
                if (!minFp || minFrameDist > frameDist) {
                    minFrameDist = frameDist;
                    minFp = fp;
                }
            }
            assert(minFp);
            auto distance = norm(desc, minFp->desc, NORM_HAMMING);
            if (distance > 100) return INITIAL_DISTANCE;
            else return distance;
            // for (auto fp : landmark->fps) {
            //     auto frame = fp->frame;
            //     if (!frame->isCurrFrame && !frame->isKeyFrame) continue;
                // cout<<"Getting distance from frame id"<<point2D->frameId<<" : "<<point2D->id<<endl;
                // double distance = norm(desc, fp->desc, NORM_HAMMING);
                // // cout<<"Landmark distance: "<<distance<<endl;
                // if (distance < minDistance) {
                //     minDistance = distance;
                // } else if (distance > 100) {
                //     return INITIAL_DISTANCE;
                // }
            // }
            return minDistance;
        }

        static double get_distance(SP<Landmark> landmark1, SP<Landmark> landmark2, SP<FrameSet> descriptorFrames) {
            double minFrameDist = -1;
            SP<FramePoint> minFp1, minFp2;
            for (auto fp1 : landmark1->fps) {
                if (descriptorFrames->count(fp1->frame) == 0) continue;
                for (auto fp2 : landmark2->fps) {
                    if (descriptorFrames->count(fp2->frame) == 0) continue;
                    auto frameDist = (fp1->frame->pose->trans - fp2->frame->pose->trans).norm();
                    if (!minFp1 || minFrameDist > frameDist) {
                        minFrameDist = frameDist;
                        minFp1 = fp1;
                        minFp2 = fp2;
                    }
                }
            }
            assert(minFp1);

            auto distance = norm(minFp1->desc, minFp2->desc, NORM_HAMMING);
            if (distance > 100) return INITIAL_DISTANCE;
            else return distance;


            // double minDistance = INITIAL_DISTANCE;
            // for (auto fp : landmark2->fps) {
            //     auto frame = fp->frame;
            //     if (!frame->isCurrFrame && !frame->isKeyFrame) continue;
            //     double distance = get_distance(landmark1, fp->desc, fp->frame);
            //     if (distance < minDistance) {
            //         minDistance = distance;
            //     } else if (distance > 100) {
            //         return INITIAL_DISTANCE;
            //     }
            // }
            // return minDistance;
        }

        static double get_distance(SP<FramePoint> fp1, SP<FramePoint> fp2, SP<FrameSet> descriptorFrames) {
            if (fp1->landmark.expired() && fp2->landmark.expired()) {
                // cout<<"Desc Dist, no landmark found"<<norm(desc1, desc2, NORM_HAMMING)<<endl;
                return norm(fp1->desc, fp2->desc, NORM_HAMMING);
            } else if (!fp1->landmark.expired()) {
                // cout<<"Desc Dist, landmark1 found"<<landmark1.get_distance(desc2)<<endl;
                return get_distance(fp1->landmark.lock(), fp2->desc, fp2->frame, descriptorFrames);
            } else if (!fp2->landmark.expired()) {
                // cout<<"Desc Dist, landmark2 found"<<landmark2.get_distance(desc1)<<endl;
                // cout<<"Landmark 2 found"<<fp1->frameId<<endl;
                return get_distance(fp2->landmark.lock(), fp1->desc, fp1->frame, descriptorFrames);
            } else {
                // cout<<"Desc Dist, landmark1 and 2 found"<<landmark1.get_distance(landmark2)<<endl;
                return get_distance(fp1->landmark.lock(), fp2->landmark.lock(), descriptorFrames);
            }
        }

        static double get_distance(SP<Landmark> landmark, SP<FramePoint> fp, SP<FrameSet> descriptorFrames) {
            if (fp->landmark.expired()) return get_distance(landmark, fp->desc, fp->frame, descriptorFrames);
            else return get_distance(landmark, fp->landmark.lock(), descriptorFrames);
        }

        static double get_distance(SP<FramePoint> fp, float px, float py) {
            return sqrt(pow(fp->x - px, 2) + pow(fp->y - py, 2));
        }

        static bool within_range(SP<FramePoint> fp, double x, double y, float inlierRange, 
                bool normalizeKP, const Mat& cameraMatrix, const Mat& distCoeffs) {
            double fpx = fp->x, fpy = fp->y;
            auto range = inlierRange;
// #if !WASM_COMPILE
            if (normalizeKP) {
                vector<cv::Point2f> keypoints{Point2f(fpx, fpy)};
                vector<cv::Point2f> undistortedKeypoints;
                cv::undistortPoints(keypoints, undistortedKeypoints, cameraMatrix, distCoeffs);
                auto undistort = undistortedKeypoints[0];
                // cout<<fp->x<<","<<fp->y<<" Undistort "<<undistort.x<<", "<<undistort.y<<endl;
                fpx = undistort.x;
                fpy = undistort.y;
                range = inlierRange/cameraMatrix.at<double>(0, 0);
            }
// #endif
            // cout<<"Within Range FP: "<<fp->frame->id<<": "<<fpx<<", "
            // cout<<fpy<<"; XY: "<<x<<", "<<y<<", InlierRange "<<inlierRange<<endl;

            // return (fpx + range > x && x + range > fpx
            //     && fpy + range > y && y + range > fpy);
            Point2f diff(fpx - x, fpy - y);
            // return make_tuple(cv::norm(diff) < range, fpx, fpy);
            return cv::norm(diff) < range;
        }

        static double gap(SP<FramePoint> fp, double x, double y, 
                bool normalizeKP, const Mat& cameraMatrix, const Mat& distCoeffs) {
            double fpx = fp->x, fpy = fp->y;
// #if !WASM_COMPILE
            if (normalizeKP) {
                vector<cv::Point2f> keypoints{Point2f(fpx, fpy)};
                vector<cv::Point2f> undistortedKeypoints;
                cv::undistortPoints(keypoints, undistortedKeypoints, cameraMatrix, distCoeffs);
                auto undistort = undistortedKeypoints[0];
                // cout<<fp->x<<","<<fp->y<<" Undistort "<<undistort.x<<", "<<undistort.y<<endl;
                fpx = undistort.x;
                fpy = undistort.y;
            }
            Point2f gap(x - fpx, y - fpy);
            return cv::norm(gap);
// #else
            // return 0;
// #endif
        }

        static bool validate_frame_trans(SP<Frame> frame, Vector3d& trans, SlamConfig& _cfg) {
            double distance = (frame->pose->trans - trans).norm();
            return distance < _cfg.maxDepth;
        }
        
        static tuple<bool, Vector3d> check_behind_frame(SP<Pose> pose, Vector3d& landmarkTrans) {
            auto eT = pose->trans;
            auto shiftedTrans = landmarkTrans - eT;
            auto rT = pose->rot.inverse() * shiftedTrans;
            return make_tuple(rT[2] <= 0, rT);
        }

        static double deg_diff(Quaterniond& q1, Quaterniond& q2) {
            q1.normalize();
            q2.normalize();

            // Compute the difference in rotation between the quaternions
            Eigen::Quaterniond q_diff = q1.inverse() * q2;

            // Convert the difference quaternion to an angle-axis representation
            Eigen::AngleAxisd angle_axis(q_diff);

            // Convert the angle of rotation to degrees
            double degree_of_rotation = angle_axis.angle() * 180 / M_PI;
            return degree_of_rotation;
        }

        static g2o::Vector2 get_projection(
            const g2o::CameraParameters* cam, const Quaterniond& poseRot, 
            const Vector3d& poseTrans, const Vector3d& kpTrans) {
            Quaterniond rot(poseRot);
            rot.normalize();
            auto diffTrans = rot.conjugate() * (kpTrans - poseTrans);
            g2o::SE3Quat originNoRot(Quaterniond::Identity(), Vector3d(0, 0, 0));
            return cam->cam_map(originNoRot.map(diffTrans));
        }

        template<typename T> static T pop_random(set<T>& tSet) {
            auto it = std::begin(tSet);
            std::advance(it, rand() % tSet.size());
            auto element = *it;
            tSet.erase(element);
            return element;
        }

        template<typename T> static T pop_random(vector<T>& vec) {
            auto it = std::begin(vec);
            std::advance(it, rand() % vec.size());
            auto element = *it;
            vec.erase(it);
            return element;
        }

        template<typename T> static T pop(vector<T>& vec, int position) {
            if (vec.size() == 0) return nullptr;
            auto it = std::begin(vec);
            std::advance(it, position);
            auto element = *it;
            vec.erase(it);
            return element;
        }
};


#endif /*__TRANSFORM_UTILS_HPP__*/