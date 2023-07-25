
#ifndef __VERTEX_SE3_CUSTOM_HPP__
#define __VERTEX_SE3_CUSTOM_HPP__

#include "g2o/core/base_vertex.h"
#include "g2o/types/slam3d/se3quat.h"
#include "g2o/types/sba/g2o_types_sba_api.h"

namespace g2o {

using namespace std;
/**
 */
class G2O_TYPES_SBA_API VertexSE3Custom : public BaseVertex<3, Vector3> {
    protected:
        SE3Quat _pose;
    public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
            VertexSE3Custom();
            
        bool read(istream& is);
    
        bool write(ostream& os) const;

        const SE3Quat& pose() const { return _pose; }

        void setPose(const SE3Quat& pose) {
            _pose = pose;
            _estimate = pose.translation();
        }

        void setToOriginImpl();

        void oplusImpl(const number_t* update_);

        // virtual bool setEstimateDataImpl(const number_t* est) {
        //     Eigen::Map<const Vector3> estMap(est);
        //     _estimate = estMap;
        //     _pose = SE3Quat(_pose.rotation(), _estimate);
        //     return true;
        // }

        // virtual bool getEstimateData(number_t* est) const {
        //     Eigen::Map<Vector3> estMap(est);
        //     estMap = _estimate;
        //     return true;
        // }

        // virtual int estimateDimension() const { return Dimension; }

        // virtual bool setMinimalEstimateDataImpl(const number_t* est) {
        //     _estimate = Eigen::Map<const Vector3>(est);
        //     return true;
        // }

        // virtual bool getMinimalEstimateData(number_t* est) const {
        //     Eigen::Map<Vector3> v(est);
        //     v = _estimate;
        //     return true;
        // }

        // virtual int minimalEstimateDimension() const { return Dimension; }
};

}  // namespace g2o

#endif /* __VERTEX_SE3_CUSTOM_HPP__ */