#include "edge_custom.hpp"

namespace g2o {
        
    EdgeCustom::EdgeCustom() 
        : BaseBinaryEdge<2, Vector2, VertexPointXYZ, VertexSE3Custom>() {
        _cam = 0;
        resizeParameters(1);
        installParameter(_cam, 0);
    };

    bool EdgeCustom::read(std::istream& is) {
        return true;
    }

    bool EdgeCustom::write(std::ostream& os) const {
        return true;
    }

    void EdgeCustom::computeError() {
        const VertexSE3Custom* v1 = static_cast<const VertexSE3Custom*>(_vertices[1]);
        const VertexPointXYZ* v2 = static_cast<const VertexPointXYZ*>(_vertices[0]);
        const CameraParameters* cam =
        static_cast<const CameraParameters*>(parameter(0));
        _error = measurement() - cam->cam_map(v1->pose().map(v2->estimate()));
    }

    // virtual void EdgeCustom::linearizeOplus() {
    //     _jacobianOplusXi = -Matrix3::Identity();
    //     _jacobianOplusXj = Matrix3::Identity();
    // }


}  // namespace g2o
