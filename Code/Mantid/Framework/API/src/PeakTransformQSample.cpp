#include "MantidAPI/PeakTransformQSample.h"
#include <boost/make_shared.hpp>

using boost::regex;

namespace Mantid {
namespace API {

PeakTransformQSample::PeakTransformQSample()
    : PeakTransform("Q_sample_x", "Q_sample_y", regex("^Q_sample_x.*$"),
                    regex("^Q_sample_y.*$"), regex("^Q_sample_z.*$")) {}

PeakTransformQSample::PeakTransformQSample(const std::string &xPlotLabel,
                                           const std::string &yPlotLabel)
    : PeakTransform(xPlotLabel, yPlotLabel, regex("^Q_sample_x.*$"),
                    regex("^Q_sample_y.*$"), regex("^Q_sample_z.*$")) {}

PeakTransformQSample::PeakTransformQSample(const PeakTransformQSample &other)
    : PeakTransform(other) {}

PeakTransformQSample &PeakTransformQSample::
operator=(const PeakTransformQSample &other) {
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

PeakTransformQSample::~PeakTransformQSample() {}

/**
Clone the PeakTransformQSample.
*/
PeakTransform_sptr PeakTransformQSample::clone() const {
  return boost::make_shared<PeakTransformQSample>(*this);
}

/** Transform peak.
@param peak : peak to transform according to internal mapping.
@return re-mapped coordinates.
*/
Mantid::Kernel::V3D
PeakTransformQSample::transformPeak(const Mantid::API::IPeak &peak) const {
  return PeakTransform::transform(peak.getQSampleFrame());
}

/**
 * @return Special coordinate system associated with this type of transform.
 */
Mantid::Kernel::SpecialCoordinateSystem
PeakTransformQSample::getCoordinateSystem() const {
  return Mantid::Kernel::QSample;
}
}
}
