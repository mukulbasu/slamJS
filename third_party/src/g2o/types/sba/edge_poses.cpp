#include "edge_poses.hpp"

namespace g2o {
        
    EdgePoses::EdgePoses() 
        : BaseBinaryEdge<3, Vector3, VertexPointXYZ, VertexPointXYZ>() {
    };

    bool EdgePoses::read(std::istream& is) {
        return true;
    }

    bool EdgePoses::write(std::ostream& os) const {
        return true;
    }

    void EdgePoses::computeError() {
        const VertexPointXYZ* v1 = static_cast<const VertexPointXYZ*>(_vertices[1]);
        const VertexPointXYZ* v0 = static_cast<const VertexPointXYZ*>(_vertices[0]);
        _error = (measurement().normalized() - (v1->estimate() - v0->estimate()).normalized());
    }

    // virtual void EdgePoses::linearizeOplus() {
    //     _jacobianOplusXi = -Matrix3::Identity();
    //     _jacobianOplusXj = Matrix3::Identity();
    // }


}  // namespace g2o
