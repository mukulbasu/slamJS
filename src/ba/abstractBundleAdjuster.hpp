/**
 * @file abstractBundleAdjuster.hpp
 * @brief Base class for bundle adjustment used by BundleAdjuster3Dof and BundleAdjuster6Dof.
 * This provides the key interfaces for BA for SLAM.
 * To perform BA, perform following steps:
 * 1. addPose: Add the camera frame poses in this step. Some might be fixed, some unfixed and hence need to be estimated.
 * 2. addKeypoint: Add the matched landmarks in this step. Some might be fixed, some unfixed and hence need to be estimated.
 * 3. addKeypointEdge: Add the frame keypoints (Framepoints) in this step. This connects the pose and key
 * 4. optimize: This runs the estimation.
 * 
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __ABSTRACT_BUNDLE_ADJUSTER_HPP__
#define __ABSTRACT_BUNDLE_ADJUSTER_HPP__

// for std
#include <iostream>
#include <map>

#include "g2o/core/sparse_optimizer.h"
#include "g2o/types/slam3d/vertex_pointxyz.h"
#include "g2o/types/slam3d/se3quat.h"
#include "../types/types.hpp"
#include "../utils/transformUtils.hpp"

using namespace std;
using namespace Eigen;
using namespace g2o;

class AbstractBundleAdjuster {
    protected:
        double _maxDepth = 100000;
        int _maxIterations = 100;
        SlamConfig& _cfg;
        SparseOptimizer* _optimizer;
        CameraParameters* _camera = NULL;

        virtual void addPoseWithEstimate(const int vertexId, 
                double x, double y, double z, Eigen::Quaterniond& rot, bool fixed) = 0;

        virtual void addLandmarkWithEstimate(size_t vertexId, 
                double x, double y, double z, bool fixed = false) = 0;

        virtual void addFramepoint(size_t landmarkVertexId, size_t poseVertexId, 
            double u, double v, int weight = 1, bool debug = false) = 0;

        virtual g2o::SE3Quat getPoseEstimateSE3Quat(int vertexId) = 0;
        
        virtual g2o::VertexPointXYZ* fetchLandmark(int vertexId) = 0;
        
        virtual void setFixedPose(int vertexId, bool fixed) = 0;
        
        virtual void setFixedKeypoint(int vertexId, bool fixed) = 0;

        SP<Pose> getPoseEstimate(size_t vertexId) {
            g2o::SE3Quat se3Quat = getPoseEstimateSE3Quat(vertexId);
            return make_shared<Pose>(se3Quat.translation(), se3Quat.rotation());
        }
        
        Eigen::Vector3d getLandmarkEstimate(size_t vertexId) {
            return fetchLandmark(vertexId)->estimate();
        }
            
    public:
        FrameSet frames;
        FramePointSet fps;
        LandmarkSet landmarks;

        AbstractBundleAdjuster(double cx, double cy, double fx, int maxIterations, double maxDepth, SlamConfig& cfg) : 
        _maxDepth(maxDepth), _maxIterations(maxIterations), _cfg(cfg), 
        _camera(new CameraParameters(fx, Eigen::Vector2d(cx, cy), 0)) {
            _camera->setId(0);
            _maxIterations = maxIterations;
            _maxDepth = maxDepth;
            // _cholmod = cholmod;

            _optimizer = new SparseOptimizer();
            _optimizer->setVerbose( false );
            _optimizer->addParameter( _camera );
        }

        /**
         * @brief Adds Camera pose to BA graph
         * 
         * @param frame 
         * @param pose 
         * @param fixed 
         */
        void addPose(SP<Frame> frame, SP<Pose> pose, bool fixed) {
            auto pos = pose->trans;
            addPoseWithEstimate(frame->id, pos[0], pos[1], pos[2], pose->rot, fixed);
            frames.insert(frame);
        }
            
        /**
         * @brief Adds Landmarks, which are matched keypoints to BA graph
         * 
         * @param landmark 
         * @param pos 
         * @param fixed 
         */
        void addLandmark(SP<Landmark> landmark, Vector3d& pos, bool fixed) {
            addLandmarkWithEstimate(landmark->id, pos[0], pos[1], pos[2], fixed);
            landmarks.insert(landmark);
        }

        /**
         * @brief Adds Framepoints, which is the edge connecting 
         * the Pose and Landmark vertices to BA graph.
         * 
         * @param fp 
         * @param landmark 
         * @param normalizeKP 
         * @param cameraMatrix 
         * @param distCoeffs 
         * @param weight 
         */
        void addFramepoint(SP<FramePoint> fp, SP<Landmark> landmark, bool normalizeKP,
                const Mat& cameraMatrix, const Mat& distCoeffs, int weight = 1) {
            double u = 0, v = 0;
            if (normalizeKP) {
                vector<cv::Point2f> keypoints{Point2f(fp->x, fp->y)};
                vector<cv::Point2f> undistortedKeypoints;
                cv::undistortPoints(keypoints, undistortedKeypoints, cameraMatrix, distCoeffs);
                auto undistort = undistortedKeypoints[0];
                //cout<<fp->x<<","<<fp->y<<" Undistort "<<undistort.x<<", "<<undistort.y<<endl;
                u = undistort.x;
                v = undistort.y;
            } else {
                u = fp->x;
                v = fp->y;
            }
            addFramepoint(landmark->id, fp->frame->id, u, v, weight, fp->frame->id == _cfg.debugFrameId);
            fps.insert(fp);
        }

        /**
         * @brief Optimizes the BA graph, thereby evaluating the pose of camera and 
         * 3D position of landmarks, such that they match the framepoint observations.
         * 
         * @param maxIterations 
         * @return int 
         */
        virtual int optimize(int maxIterations = -1) = 0;

        virtual void optimizeVertices() = 0;

        /**
         * @brief Get the camera Pose (rotation and translation) estimate
         * 
         * @param frame 
         * @return SP<Pose> 
         */
        SP<Pose> getPoseEstimate(SP<Frame> frame) {
            return getPoseEstimate(frame->id);
        }

        /**
         * @brief Get the Landmark translation estimate
         * 
         * @param landmark 
         * @return Eigen::Vector3d 
         */
        Eigen::Vector3d getLandmarkEstimate(SP<Landmark> landmark) {
            return getLandmarkEstimate(landmark->id);
        }

        /**
         * @brief Extract both camera and landmark estimates
         * 
         * @return tuple<SP<LandmarkTransMap>, SP<FramePoseMap>> 
         */
        tuple<SP<LandmarkTransMap>, SP<FramePoseMap>> get_estimates() 
        {
            auto framePoseMap = make_shared<FramePoseMap>();
            auto landmarkEstimateMap = make_shared<LandmarkTransMap>();
            for (auto frame : frames) {
                (*framePoseMap)[frame] = getPoseEstimate(frame);
            }
            
            for (auto landmark : landmarks) {
                (*landmarkEstimateMap)[landmark] = getLandmarkEstimate(landmark);
            }

            return make_tuple(landmarkEstimateMap, framePoseMap);
        }

        /**
         * @brief Get the U,V or X, Y 2D projection of a 3D landmark point on 
         * a camera given its pose (position and rotation)
         * 
         * @param pose 
         * @param landmarkPos 
         * @return tuple<double, double> 
         */
        tuple<double, double> getProjection(
                SP<Pose> pose, const Vector3d& landmarkPos) {
            g2o::Vector2 estimate;
            estimate = TransformUtils::get_projection(
                        _camera, pose->rot, pose->trans, landmarkPos);
            return make_tuple(estimate[0], estimate[1]);
        }

        /**
         * @brief Get the Camera Intrinsic params
         * 
         * @return CameraParameters* 
         */
        CameraParameters* getCameraParams() { return _camera;}

        /**
         * @brief Fix this camera frame pose. 
         * This will not be changed during BA optimization. Other camera poses and landmarks
         * will be computed with respect to the fixed camera poses and landmarks.
         * 
         * @param frame 
         * @param fixed 
         */
        void setFixed(SP<Frame> frame, bool fixed) {
            setFixedPose(frame->id, fixed);
        }

        /**
         * @brief Fix this landmark. This will not be changed during BA optimization.
         * Other camera poses and landmarks will be computed with respect to the fixed
         * camera poses and landmarks.
         * 
         * @param landmark 
         * @param fixed 
         */
        void setFixed(SP<Landmark> landmark, bool fixed) {
            setFixedKeypoint(landmark->id, fixed);
        }
};

using BA = AbstractBundleAdjuster;

#endif /* #define __ABSTRACT_BUNDLE_ADJUSTER_HPP__ */