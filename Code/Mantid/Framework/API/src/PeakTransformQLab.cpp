#include "MantidAPI/PeakTransformQLab.h"
#include <boost/make_shared.hpp>

using boost::regex;

namespace Mantid {
namespace API {

PeakTransformQLab::PeakTransformQLab()
    : PeakTransform("Q_lab_x", "Q_lab_y", regex("^Q_lab_x.*$"),
                    regex("^Q_lab_y.*$"), regex("^Q_lab_z.*$")) {}

PeakTransformQLab::PeakTransformQLab(const std::string &xPlotLabel,
                                     const std::string &yPlotLabel)
    : PeakTransform(xPlotLabel, yPlotLabel, regex("^Q_lab_x.*$"),
                    regex("^Q_lab_y.*$"), regex("^Q_lab_z.*$")) {}

PeakTransformQLab::PeakTransformQLab(const PeakTransformQLab &other)
    : PeakTransform(other) {}

PeakTransformQLab &PeakTransformQLab::
operator=(const PeakTransformQLab &other) {
  if (this != &other) {
    m_xPlotLabel = other.m_xPlotLabel;
    m_yPlotLabel = other.m_yPlotLabel;
    m_indexOfPlotX = other.m_indexOfPlotX;
    m_indexOfPlotY = other.m_indexOfPlotY;
    m_indexOfPlotZ = other.m_indexOfPlotZ;
    m_FirstRegex = other.m_FirstRegex;
    m_SecondRegex = other.m_SecondRegex;
    m_ThirdRegex = other.m_ThirdRegex;
  }
  return *this;
}

PeakTransformQLab::~PeakTransformQLab() {}

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
PeakTransformQLab::transformPeak(const Mantid::API::IPeak &peak) const {
  return PeakTransform::transform(peak.getQLabFrame());
}

/**
 * @return Special coordinate system associated with this type of transform.
 */
Mantid::API::SpecialCoordinateSystem
PeakTransformQLab::getCoordinateSystem() const {
  return Mantid::API::QLab;
}
}
}
