#ifndef MANTID_GEOMETRY_PEAKTRANSFORMQSAMPLE_H_
#define MANTID_GEOMETRY_PEAKTRANSFORMQSAMPLE_H_

#include "MantidGeometry/Crystal/ConcretePeakTransformFactory.h"
#include "MantidGeometry/Crystal/PeakTransform.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Geometry {
/**
@class PeakTransformQSample
Used to remap coordinates into a form consistent with an axis reordering.
*/
class DLLExport PeakTransformQSample : public PeakTransform {
public:
  /// Transform name.
  static std::string name() { return "Q (sample frame)"; }
  /// Constructor
  PeakTransformQSample();
  /// Constructor
  PeakTransformQSample(const std::string &xPlotLabel,
                       const std::string &yPlotLabel);
  PeakTransform_sptr clone() const override;
  /// Transform peak.
  Mantid::Kernel::V3D
  transformPeak(const Mantid::Geometry::IPeak &peak) const override;
  /// Getter for the transform name.
  std::string getFriendlyName() const override { return name(); }
  /// Getter for the special coordinate representation of this transform type.
  Mantid::Kernel::SpecialCoordinateSystem getCoordinateSystem() const override;
};

/// Typedef a factory for type of PeaksTransform.
using PeakTransformQSampleFactory =
    ConcretePeakTransformFactory<PeakTransformQSample>;
} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_PEAKTRANSFORMQSAMPLE_H_ */
