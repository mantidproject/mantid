// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/PeakTransformQSample.h"
#include <memory>

using boost::regex;

namespace Mantid {
namespace Geometry {

PeakTransformQSample::PeakTransformQSample()
    : PeakTransform("Q_sample_x", "Q_sample_y", regex("^Q_sample_x.*$"), regex("^Q_sample_y.*$"),
                    regex("^Q_sample_z.*$")) {}

PeakTransformQSample::PeakTransformQSample(const std::string &xPlotLabel, const std::string &yPlotLabel)
    : PeakTransform(xPlotLabel, yPlotLabel, regex("^Q_sample_x.*$"), regex("^Q_sample_y.*$"), regex("^Q_sample_z.*$")) {
}

/**
Clone the PeakTransformQSample.
*/
PeakTransform_sptr PeakTransformQSample::clone() const { return std::make_shared<PeakTransformQSample>(*this); }

/** Transform peak.
@param peak : peak to transform according to internal mapping.
@return re-mapped coordinates.
*/
Mantid::Kernel::V3D PeakTransformQSample::transformPeak(const Mantid::Geometry::IPeak &peak) const {
  return PeakTransform::transform(peak.getQSampleFrame());
}

/**
 * @return Special coordinate system associated with this type of transform.
 */
Mantid::Kernel::SpecialCoordinateSystem PeakTransformQSample::getCoordinateSystem() const {
  return Mantid::Kernel::QSample;
}
} // namespace Geometry
} // namespace Mantid
