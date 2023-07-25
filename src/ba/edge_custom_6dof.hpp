/**
 * @file edge_custom_6dof.hpp
 * @brief Extends the standard EdgeProjectXYZ2UV class to add some debug functionalities as well as custom error computation logic.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __EdgeCustom6Dof_HPP__
#define __EdgeCustom6Dof_HPP__

#include "g2o/types/sba/edge_project_xyz2uv.h"
#include "../utils/transformUtils.hpp"

namespace g2o {

class EdgeCustom6Dof : public EdgeProjectXYZ2UV {
    protected:
        bool _debug = false;
        float _K = 180*7/22;

    public:
        EdgeCustom6Dof(bool debug = false) : _debug(debug) {}

        void computeError() {
            const VertexSE3Expmap* v1 = static_cast<const VertexSE3Expmap*>(_vertices[1]);
            const VertexPointXYZ* v2 = static_cast<const VertexPointXYZ*>(_vertices[0]);
            const CameraParameters* cam =
            static_cast<const CameraParameters*>(parameter(0));
            auto kpTrans = v2->estimate();
            auto pTrans = v1->estimate().translation();
            auto pRot = v1->estimate().rotation();
            auto pEuler = pRot.toRotationMatrix().eulerAngles(1, 0, 2);
            auto projection = TransformUtils::get_projection(cam, pRot, pTrans, kpTrans);
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

#endif /* __EdgeCustom6Dof_HPP__ */