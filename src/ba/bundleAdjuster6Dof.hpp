/**
 * @file bundleAdjuster6Dof.hpp
 * @brief Bundle Adjustment of camera poses and landmarks for SLAM.
 * This evaluates both rotation and position of camera as well as position of landmarks. 
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __BUNDLE_ADJUSTER_6DOF_HPP__
#define __BUNDLE_ADJUSTER_6DOF_HPP__

// for std
#include <iostream>
#include <map>
// for opencv 
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
// #include <boost/concept_check.hpp>
// for g2o
#include <g2o/core/base_edge.h>
#include <g2o/core/sparse_optimizer.h>
#include <g2o/core/block_solver.h>
#include <g2o/core/base_vertex.h>
#include <g2o/core/robust_kernel.h>
#include <g2o/core/robust_kernel_impl.h>
#include <g2o/core/optimization_algorithm_levenberg.h>
// #include <g2o/solvers/cholmod/linear_solver_cholmod.h>
#include "g2o/solvers/structure_only/structure_only_solver.h"
#include "g2o/solvers/eigen/linear_solver_eigen.h"
#include "g2o/types/sba/types_six_dof_expmap.h"
#include <g2o/stuff/sampler.h>

#include "../types/types.hpp"
#include "abstractBundleAdjuster.hpp"
#include "edge_custom_6dof.hpp"

using namespace std;
using namespace Eigen;
using namespace g2o;

class BundleAdjuster6Dof : public AbstractBundleAdjuster {
    
    protected:
        // bool _cholmod = true;
        std::unique_ptr<g2o::BlockSolver_6_3::LinearSolverType> _linearSolver;
        g2o::OptimizationAlgorithmLevenberg* _algorithm;

        map<size_t, vector<EdgeCustom6Dof*>*> _framepointEdges;
        map<size_t, g2o::VertexSE3Expmap*> _poseVertices;
        map<size_t, g2o::VertexPointXYZ*> _landmarkVertices;

        void clearAll() {
            try {
                for (auto const& elem : _framepointEdges) {
                    elem.second->clear();
                    delete( elem.second );
                }
                _framepointEdges.clear();
                _poseVertices.clear();
                _landmarkVertices.clear();
            } catch (const std::exception &exc) {
                std::cerr << exc.what();
            }
        }

        g2o::VertexSE3Expmap* fetchPose(int vertexId) {
            return dynamic_cast<g2o::VertexSE3Expmap*>( _optimizer->vertex( vertexId ) );
        }

        g2o::VertexPointXYZ* fetchLandmark(int vertexId) {
            return dynamic_cast<g2o::VertexPointXYZ*>( _optimizer->vertex( vertexId ) );
        }

        g2o::SE3Quat getPoseEstimateSE3Quat(int vertexId) {
            g2o::VertexSE3Expmap* v = fetchPose( vertexId );
            return v->estimate();
        }

        void add_invalid_pose() {
            // cout<<"Adding Invalid pose and vertices"<<endl;
            int maxId = 0;
            for (auto const& [vertexId, v] : _poseVertices) {
                if (maxId < (int)vertexId) maxId = vertexId;
            }
            maxId = maxId + 1000;
            Eigen::Quaterniond rot;
            rot.setIdentity();
            addPoseWithEstimate(maxId+1, 0, 0, 0, rot, false);
            _poseVertices.erase(maxId+1);
            // _poseIds.erase(maxId+1);
            int kpId = maxId+2;
            addLandmarkWithEstimate(kpId, 0, 0, _maxDepth);
            _landmarkVertices.erase(kpId);
            // _kpIds.erase(kpId);
            addFramepoint(kpId, maxId, 0, 0);
            addFramepoint(kpId, maxId+1, 0, 0);
        }

    public:
        
        BundleAdjuster6Dof(double cx, double cy, double fx, int maxIterations, double maxDepth, bool cholmod, SlamConfig& cfg)
            : AbstractBundleAdjuster(cx, cy, fx, maxIterations, maxDepth, cfg) {
            // using BaLinearCholmodSolver = LinearSolverCholmod<BlockSolverPL<6, 3>::PoseMatrixType>;
            using BaLinearEigenSolver = LinearSolverEigen<BlockSolverPL<6, 3>::PoseMatrixType>;
            // if (_cholmod) {
            //     _linearSolver = g2o::make_unique<BaLinearCholmodSolver>();
            // } else {
                _linearSolver = g2o::make_unique<BaLinearEigenSolver>();
            // }
            _algorithm = new g2o::OptimizationAlgorithmLevenberg(
                g2o::make_unique<g2o::BlockSolver_6_3>(std::move( _linearSolver ))
            );

            _optimizer->setAlgorithm( _algorithm );
        }

        ~BundleAdjuster6Dof() {
            clearAll();
        }

        void addPoseWithEstimate(const int vertexId, double x, double y, double z, Eigen::Quaterniond& rot, bool fixed) {
            g2o::VertexSE3Expmap* v = new g2o::VertexSE3Expmap();
            v->setId(vertexId);
            v->setFixed(fixed);
            g2o::Vector3 trans = g2o::Vector3(x, y, z);
            v->setEstimate(g2o::SE3Quat(rot, trans));
            _optimizer->addVertex(v);
            _poseVertices[vertexId] = v;
        }

        void addLandmarkWithEstimate(size_t vertexId, double x, double y, double z, bool fixed = false) {
            g2o::VertexPointXYZ* v = new g2o::VertexPointXYZ();
            v->setId( vertexId );
            v->setMarginalized(true);
            //Bundle adjustment fails if depth is zero
            assert(z != 0 || x != 0 || y != 0);
            v->setEstimate( Eigen::Vector3d(x, y, z) );
            v->setFixed(fixed);
            _optimizer->addVertex( v );
            _landmarkVertices[vertexId] = v;
        }

        void addFramepoint(size_t kpVertexId, size_t poseVertexId, double u, double v, int weight = 1, bool debug = false) {
            EdgeCustom6Dof*  edge = new EdgeCustom6Dof(debug);
            edge->setVertex( 0, fetchLandmark(kpVertexId));
            edge->setVertex( 1, fetchPose(poseVertexId));
            edge->setMeasurement( Eigen::Vector2d(u, v) );
            edge->setInformation( Eigen::Matrix2d::Identity() * weight);
            edge->setParameterId(0, 0);
            edge->setRobustKernel( new g2o::RobustKernelHuber() );
            _optimizer->addEdge( edge );
            if (_framepointEdges.find(kpVertexId) != _framepointEdges.end()) {
                _framepointEdges.at(kpVertexId)->push_back(edge);
            } else {
                vector<EdgeCustom6Dof*>* vect = new vector<EdgeCustom6Dof*>;
                vect->push_back(edge);
                _framepointEdges[kpVertexId] = vect;
            }
            // cout<< "Adding edge "<<kpVertexId<<":"<<poseVertexId<<" coord:"<<u<<", "<<v<<endl;
        }

        bool addedInvalid = false;
        int optimize(int maxIterations = -1) {
            //Check if at least one pose is unfixed. Otherwise, 
            // BA will fail as there is nothing to predict.
            bool atleast1poseunfixed = false;
            for (auto const& [vertexId, v] : _poseVertices) {
                if (!v->fixed()) {
                    atleast1poseunfixed = true;
                    break;
                }
            }
            
            // If no poses are unfixed, (because only landmarks need to be estimated),
            // add a dummy pose to enable BA to run
            if (!atleast1poseunfixed && !addedInvalid) {
                add_invalid_pose();
                addedInvalid = true;
                atleast1poseunfixed = true;
            }
            _optimizer->initializeOptimization();
            if (maxIterations == -1) maxIterations = _maxIterations;
            return _optimizer->optimize(maxIterations);
        }

        void optimizeVertices() {
            g2o::StructureOnlySolver<3> structure_only_ba;
            cout << "Performing structure-only BA:" << endl;
            g2o::OptimizableGraph::VertexContainer points;
            for (g2o::OptimizableGraph::VertexIDMap::const_iterator it =
                    _optimizer->vertices().begin();
                it != _optimizer->vertices().end(); ++it) {
            g2o::OptimizableGraph::Vertex* v =
                static_cast<g2o::OptimizableGraph::Vertex*>(it->second);
            if (v->dimension() == 3) points.push_back(v);
            }
            structure_only_ba.calc(points, 10);
        }

        void setFixedPose(int vertexId, bool fixed) {
            _poseVertices[vertexId]->setFixed(fixed);
        }

        void setFixedKeypoint(int vertexId, bool fixed) {
            _landmarkVertices[vertexId]->setFixed(fixed);
        }
};

#endif /* __BUNDLE_ADJUSTER_6DOF_HPP__ */