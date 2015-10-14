#ifndef MANTID_GEOMETRY_PEAKTRANSFORMQSAMPLE_H_
#define MANTID_GEOMETRY_PEAKTRANSFORMQSAMPLE_H_

#include "MantidKernel/System.h"
#include "MantidGeometry/Crystal/PeakTransform.h"
#include "MantidGeometry/Crystal/ConcretePeakTransformFactory.h"

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
  /// Destructor
  virtual ~PeakTransformQSample();
  /// Copy constructor
  PeakTransformQSample(const PeakTransformQSample &other);
  /// Assigment
  PeakTransformQSample &operator=(const PeakTransformQSample &other);
  /// Virtual constructor
  PeakTransform_sptr clone() const;
  /// Transform peak.
  Mantid::Kernel::V3D transformPeak(const Mantid::Geometry::IPeak &peak) const;
  /// Getter for the transform name.
  virtual std::string getFriendlyName() const { return name(); }
  /// Getter for the special coordinate representation of this transform type.
  Mantid::Kernel::SpecialCoordinateSystem getCoordinateSystem() const;
};

/// Typedef a factory for type of PeaksTransform.
typedef ConcretePeakTransformFactory<PeakTransformQSample>
    PeakTransformQSampleFactory;
}
}

#endif /* MANTID_GEOMETRY_PEAKTRANSFORMQSAMPLE_H_ */
