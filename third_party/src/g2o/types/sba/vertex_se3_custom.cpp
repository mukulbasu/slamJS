#include "vertex_se3_custom.hpp"

namespace g2o {

using namespace std;

VertexSE3Custom::VertexSE3Custom() : BaseVertex<3, Vector3>() {}
        
bool VertexSE3Custom::read(istream& is) {
    return internal::readVector(is, _estimate);
}

bool VertexSE3Custom::write(ostream& os) const {
    return internal::writeVector(os, estimate());
}

    
void VertexSE3Custom::setToOriginImpl() { 
    _estimate.fill(0.);
    _pose = SE3Quat();
}

void VertexSE3Custom::oplusImpl(const number_t* update_) {
    Eigen::Map<const Vector3> update(update_);
    _estimate += update;

    _pose = SE3Quat(_pose.rotation(), _estimate);
}

}  // namespace g2o