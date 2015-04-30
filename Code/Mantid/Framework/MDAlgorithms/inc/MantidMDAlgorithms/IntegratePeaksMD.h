#ifndef MANTID_MDALGORITHMS_INTEGRATEPEAKSMD_H_
#define MANTID_MDALGORITHMS_INTEGRATEPEAKSMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidAPI/CompositeFunction.h"

namespace Mantid {
namespace MDAlgorithms {

/** Integrate single-crystal peaks in reciprocal-space.
 *
 * @author Janik Zikovsky
 * @date 2011-04-13 18:11:53.496539
 */
class DLLExport IntegratePeaksMD : public API::Algorithm {
public:
  IntegratePeaksMD();
  ~IntegratePeaksMD();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "IntegratePeaksMD"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Integrate single-crystal peaks in reciprocal space, for "
           "MDEventWorkspaces.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MDAlgorithms"; }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();

  template <typename MDE, size_t nd>
  void integrate(typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

  /// Input MDEventWorkspace
  Mantid::API::IMDEventWorkspace_sptr inWS;

  /// Calculate if this Q is on a detector
  bool detectorQ(Mantid::Kernel::V3D QLabFrame, double PeakRadius);

  /// Instrument reference
  Geometry::Instrument_const_sptr inst;
};

} // namespace Mantid
} // namespace DataObjects

#endif /* MANTID_MDALGORITHMS_INTEGRATEPEAKSMD_H_ */
