/**
 * @file edge_custom_3dof.hpp
 * @brief Extends the EdgeCustom class to add some debug functionalities as well as custom error computation logic.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __EDGE_CUSTOM_3DOF_HPP__
#define __EDGE_CUSTOM_3DOF_HPP__

#include "g2o/types/sba/edge_custom.hpp"
#include "../utils/transformUtils.hpp"

namespace g2o {

class EdgeCustom3Dof : public EdgeCustom {
    protected:
        bool _debug = false;
        float _K = 180*7/22;

    public:
        EdgeCustom3Dof(bool debug = false) : _debug(debug) {}

        void computeError() {
            const VertexSE3Custom* v1 = static_cast<const VertexSE3Custom*>(_vertices[1]);
            const VertexPointXYZ* v2 = static_cast<const VertexPointXYZ*>(_vertices[0]);
            const CameraParameters* cam =
            static_cast<const CameraParameters*>(parameter(0));
            auto kpTrans = v2->estimate();
            auto pTrans = v1->estimate();
            auto pRot = v1->pose().rotation();
            auto pEuler = pRot.toRotationMatrix().eulerAngles(1, 0, 2);
            auto projection = TransformUtils::get_projection(cam, pRot, pTrans, kpTrans);
            // auto projection = cam->cam_map(v1->pose().map(v2->estimate()));
            _error = measurement() - projection;
            if (_debug) {
                std::cout<<v2->id()<<": Error "<<_error[0]<<", "<<_error[1]<<" | ";
                std::cout<<measurement()[0]<<", "<<measurement()[1]<<" | ";
                std::cout<<projection[0]<<", "<<projection[1]<<" | ";
                std::cout<<kpTrans[0]<<", "<<kpTrans[1]<<", "<<kpTrans[2]<<" | ";
                std::cout<<pTrans[0]<<", "<<pTrans[1]<<", "<<pTrans[2]<<" | ";
                std::cout<<pEuler[1]*_K<<", "<<pEuler[0]*_K<<", "<<pEuler[2]*_K<<std::endl;
            }
        }
};

}  // namespace g2o

#endif /* __EDGE_CUSTOM_3DOF_HPP__ */