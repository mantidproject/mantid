#ifndef MANTID_API_PeakTransformQLab_H_
#define MANTID_API_PeakTransformQLab_H_

#include "MantidAPI/PeakTransform.h"
#include "MantidAPI/ConcretePeakTransformFactory.h"

namespace Mantid {
namespace API {
/**
@class PeakTransformQLab
Used to remap coordinates into a form consistent with an axis reordering.
*/
class DLLExport PeakTransformQLab : public PeakTransform {
public:
  static std::string name() { return "Q (lab frame)"; }
  /// Constructor
  PeakTransformQLab();
  /// Constructor
  PeakTransformQLab(const std::string &xPlotLabel,
                    const std::string &yPlotLabel);
  /// Destructor
  virtual ~PeakTransformQLab();
  /// Copy constructor
  PeakTransformQLab(const PeakTransformQLab &other);
  /// Assigment
  PeakTransformQLab &operator=(const PeakTransformQLab &other);
  /// Virtual constructor
  PeakTransform_sptr clone() const;
  /// Transform peak.
  Mantid::Kernel::V3D transformPeak(const Mantid::API::IPeak &peak) const;
  /// Get the transform friendly name.
  virtual std::string getFriendlyName() const { return name(); }
  /// Getter for the special coordinate representation of this transform type.
  Mantid::API::SpecialCoordinateSystem getCoordinateSystem() const;
};

/// Typedef a factory for type of PeaksTransform.
typedef ConcretePeakTransformFactory<PeakTransformQLab>
    PeakTransformQLabFactory;
}
}

#endif /* MANTID_API_PeakTransformQLab_H_ */
