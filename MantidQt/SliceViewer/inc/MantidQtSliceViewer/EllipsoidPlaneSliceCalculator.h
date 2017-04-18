#ifndef MANTID_SLICEVIEWER_ELLIPSOID_PLANE_SLICE_CALCULATOR_H_
#define MANTID_SLICEVIEWER_ELLIPSOID_PLANE_SLICE_CALCULATOR_H_

#include "DllOption.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"
#include "MantidQtSliceViewer/PeakBoundingBox.h"
#include <vector>

namespace Mantid {
namespace SliceViewer {

template <class T>
typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type
    EXPORT_OPT_MANTIDQT_SLICEVIEWER
    almost_equal(T x, T y) {
  return std::abs(x - y) <
             std::numeric_limits<T>::epsilon() * std::abs(x + y) ||
         std::abs(x - y) < std::numeric_limits<T>::min();
}

Mantid::Kernel::Matrix<double> EXPORT_OPT_MANTIDQT_SLICEVIEWER
createEllipsoidMatrixInXYZFrame(std::vector<Mantid::Kernel::V3D> directions,
                                std::vector<double> radii);

bool EXPORT_OPT_MANTIDQT_SLICEVIEWER
checkIfCutExists(const std::vector<Mantid::Kernel::V3D> &directions,
                 const std::vector<double> &radii,
                 const Mantid::Kernel::V3D &originEllipsoid, double zPlane);

std::vector<double> EXPORT_OPT_MANTIDQT_SLICEVIEWER
getProjectionLengths(const std::vector<Mantid::Kernel::V3D> &directions,
                     std::vector<double> radii);

MantidQt::SliceViewer::PeakBoundingBox EXPORT_OPT_MANTIDQT_SLICEVIEWER
getPeakBoundingBoxForEllipsoid(
    const std::vector<Mantid::Kernel::V3D> &directions,
    const std::vector<double> &radii,
    const Mantid::Kernel::V3D &originEllipsoid);

struct EXPORT_OPT_MANTIDQT_SLICEVIEWER SliceEllipseInfo {
  SliceEllipseInfo(Mantid::Kernel::V3D origin = Mantid::Kernel::V3D(0, 0, 0),
                   double radiusMajorAxis = 0.0, double radiusMinorAxis = 0.0,
                   double angle = 0.0)
      : origin(origin), radiusMajorAxis(radiusMajorAxis),
        radiusMinorAxis(radiusMinorAxis), angle(angle) {}
  Mantid::Kernel::V3D origin;
  double radiusMajorAxis;
  double radiusMinorAxis;
  double angle;
};

class EXPORT_OPT_MANTIDQT_SLICEVIEWER EllipsoidPlaneSliceCalculator {
public:
  SliceEllipseInfo
  getSlicePlaneInfo(std::vector<Mantid::Kernel::V3D> directions,
                    std::vector<double> radii,
                    Mantid::Kernel::V3D originEllipsoid, double zPlane) const;
  double getZoomOutFactor() const;

private:
  SliceEllipseInfo
  getSolutionForEllipsoid(const Kernel::Matrix<double> &m, double zPlane,
                          Mantid::Kernel::V3D originEllipsoid) const;

  bool checkIfIsEllipse(const Kernel::Matrix<double> &m) const;
  bool checkIfIsCircle(const Kernel::Matrix<double> &m) const;
  const double m_zoomOutFactor = 2.;
};
}
}

#endif
