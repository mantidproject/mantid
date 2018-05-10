#include "MantidGeometry/Crystal/PeakTransformQLab.h"
#include <boost/make_shared.hpp>

using boost::regex;

namespace Mantid {
namespace Geometry {

PeakTransformQLab::PeakTransformQLab()
    : PeakTransform("Q_lab_x", "Q_lab_y", regex("^Q_lab_x.*$"),
                    regex("^Q_lab_y.*$"), regex("^Q_lab_z.*$")) {}

PeakTransformQLab::PeakTransformQLab(const std::string &xPlotLabel,
                                     const std::string &yPlotLabel)
    : PeakTransform(xPlotLabel, yPlotLabel, regex("^Q_lab_x.*$"),
                    regex("^Q_lab_y.*$"), regex("^Q_lab_z.*$")) {}

/**
Clone the PeakTransformQLab.
*/
PeakTransform_sptr PeakTransformQLab::clone() const {
  return boost::make_shared<PeakTransformQLab>(*this);
}

/** Transform peak.
@param peak : peak to transform according to internal mapping.
@return re-mapped coordinates.
*/
Mantid::Kernel::V3D
PeakTransformQLab::transformPeak(const Mantid::Geometry::IPeak &peak) const {
  return PeakTransform::transform(peak.getQLabFrame());
}

/**
 * @return Special coordinate system associated with this type of transform.
 */
Mantid::Kernel::SpecialCoordinateSystem
PeakTransformQLab::getCoordinateSystem() const {
  return Mantid::Kernel::QLab;
}
} // namespace Geometry
} // namespace Mantid
