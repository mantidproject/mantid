#ifndef MANTID_SLICEVIEWER_ELLIPSOID_PLANE_SLICE_CALCULATOR_H_
#define MANTID_SLICEVIEWER_ELLIPSOID_PLANE_SLICE_CALCULATOR_H_

#include "DllOption.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Matrix.h"
#include <vector>

namespace Mantid
{
namespace SliceViewer
{

Mantid::Kernel::Matrix<double> EXPORT_OPT_MANTIDQT_SLICEVIEWER
createEllipsoidMatrixInXYZFrame(std::vector<Mantid::Kernel::V3D> directions,
                                std::vector<double> radii);

struct SliceEllipseInfo {
    SliceEllipseInfo(Mantid::Kernel::V3D origin, double radius1, double radius2,
                     double angle)
        : origin(origin), radius1(radius1), radius2(radius2), angle(angle)
    {
    }

    Mantid::Kernel::V3D origin;
    double radius1;
    double radius2;
    double angle;
};

class EXPORT_OPT_MANTIDQT_SLICEVIEWER EllipsoidPlaneSliceCalculator
{
public:
    SliceEllipseInfo
    getSlicePlaneInfo(std::vector<Mantid::Kernel::V3D> directions,
                      std::vector<double> radii,
                      Mantid::Kernel::V3D originEllipsoid, double zPlane);

private:
    SliceEllipseInfo
    getSolutionForEllipsoid(const Kernel::Matrix<double> &m, double zPlane,
                            Mantid::Kernel::V3D originEllipsoid) const;

    bool checkIfIsEllipse(const Kernel::Matrix<double> &m) const;
    bool checkIfIsCircle(const Kernel::Matrix<double> &m) const;
};
}
}

#endif
