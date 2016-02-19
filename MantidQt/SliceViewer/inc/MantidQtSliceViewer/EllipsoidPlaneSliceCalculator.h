#ifndef MANTID_SLICEVIEWER_ELLIPSOID_PLANE_SLICE_CALCULATOR_H_
#define MANTID_SLICEVIEWER_ELLIPSOID_PLANE_SLICE_CALCULATOR_H_

#include "DllOption.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/V2D.h"
#include "MantidKernel/Matrix.h"
#include <vector>

namespace Mantid
{
namespace SliceViewer
{

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
                      std::vector<Mantid::Kernel::V3D> radii,
                      Mantid::Kernel::V3D originEllipsoid, double zPlane);

private:
    void setupEllipsoidMatrix(Kernel::Matrix<double> &m, std::vector<Mantid::Kernel::V3D> directions,
                              std::vector<Mantid::Kernel::V3D> radii);
    Mantid::Kernel::V3D getOriginOfSliceEllipse(const Kernel::Matrix<double> &m,
                                                double zPlane);
    Mantid::Kernel::V2D getRadii(const Kernel::Matrix<double> &m,
                                 double zPlane);
    double getAngle();

    double calculateAngle(const Kernel::Matrix<double> &m) const;

    void setSquareRootPrefactor(const Kernel::Matrix<double> &m);
    void setSinValue(double angle);
    void setCosValue(double angle);

    void setupCache(const Kernel::Matrix<double> &m);

    double m_angle;
    double m_sinValue;
    double m_cosValue;
    double m_squareRootPrefactor;
};
}
}

#endif
