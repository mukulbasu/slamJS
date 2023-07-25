#ifndef __EDGE_POSES_HPP__
#define __EDGE_POSES_HPP__

#include "g2o/core/base_binary_edge.h"
#include "g2o/types/slam3d/vertex_pointxyz.h"
#include "g2o/types/sba/g2o_types_sba_api.h"
#include "g2o/types/sba/parameter_cameraparameters.h"
#include "g2o/types/sba/vertex_se3_custom.hpp"

namespace g2o {

class G2O_TYPES_SBA_API EdgePoses
    : public BaseBinaryEdge<3, g2o::Vector3, VertexPointXYZ, VertexPointXYZ> {
    public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
        
        EdgePoses();

        bool read(std::istream& is);

        bool write(std::ostream& os) const;

        void computeError();

        // virtual void linearizeOplus() {
        //     _jacobianOplusXi = -Matrix3::Identity();
        //     _jacobianOplusXj = Matrix3::Identity();
        // }

};

}  // namespace g2o

#endif /* __EDGE_POSES_HPP__ */