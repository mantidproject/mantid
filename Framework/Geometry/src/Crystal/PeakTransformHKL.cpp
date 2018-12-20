#include "MantidGeometry/Crystal/PeakTransformHKL.h"
#include <boost/make_shared.hpp>

using boost::regex;

namespace Mantid {
namespace Geometry {

PeakTransformHKL::PeakTransformHKL()
    : PeakTransform("H", "K", regex("^(H.*)|(\\[H,0,0\\].*)$"),
                    regex("^(K.*)|(\\[0,K,0\\].*)$"),
                    regex("^(L.*)|(\\[0,0,L\\].*)$")) {}

PeakTransformHKL::PeakTransformHKL(const std::string &xPlotLabel,
                                   const std::string &yPlotLabel)
    : PeakTransform(xPlotLabel, yPlotLabel, regex("^(H.*)|(\\[H,0,0\\].*)$"),
                    regex("^(K.*)|(\\[0,K,0\\].*)$"),
                    regex("^(L.*)|(\\[0,0,L\\].*)$")) {}

/**
Clone the PeakTransformHKL.
*/
PeakTransform_sptr PeakTransformHKL::clone() const {
  return boost::make_shared<PeakTransformHKL>(*this);
}

/** Transform peak.
@param peak : peak to transform according to internal mapping.
@return re-mapped coordinates.
*/
Mantid::Kernel::V3D
PeakTransformHKL::transformPeak(const Mantid::Geometry::IPeak &peak) const {
  return PeakTransform::transform(peak.getHKL());
}

/**
 * @return Special coordinate system associated with this type of transform.
 */
Mantid::Kernel::SpecialCoordinateSystem
PeakTransformHKL::getCoordinateSystem() const {
  return Mantid::Kernel::HKL;
}
} // namespace Geometry
} // namespace Mantid
