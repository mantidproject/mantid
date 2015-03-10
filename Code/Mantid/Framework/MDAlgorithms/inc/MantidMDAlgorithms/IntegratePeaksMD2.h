#ifndef MANTID_MDALGORITHMS_INTEGRATEPEAKSMD_H_
#define MANTID_MDALGORITHMS_INTEGRATEPEAKSMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidAPI/CompositeFunction.h"

namespace Mantid {
namespace MDAlgorithms {

/** Integrate single-crystal peaks in reciprocal-space.
 *
 * @author Janik Zikovsky
 * @date 2011-04-13 18:11:53.496539
 */
class DLLExport IntegratePeaksMD2 : public API::Algorithm {
public:
  IntegratePeaksMD2();
  ~IntegratePeaksMD2();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "IntegratePeaksMD"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Integrate single-crystal peaks in reciprocal space, for "
           "MDEventWorkspaces.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 2; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MDAlgorithms"; }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();

  template <typename MDE, size_t nd>
  void integrate(typename MDEvents::MDEventWorkspace<MDE, nd>::sptr ws);

  /// Input MDEventWorkspace
  Mantid::API::IMDEventWorkspace_sptr inWS;

  /// Calculate if this Q is on a detector
  void calculateE1(Geometry::Instrument_const_sptr inst) ;
  bool detectorQ(Mantid::Kernel::V3D QLabFrame, double PeakRadius);
  void runMaskDetectors(Mantid::DataObjects::PeaksWorkspace_sptr peakWS,
                        std::string property, std::string values);

  /// save for all detector pixels
  std::vector<Kernel::V3D> E1Vec;

  /// Check if peaks overlap
  void checkOverlap(int i, Mantid::DataObjects::PeaksWorkspace_sptr peakWS,
                    Mantid::Kernel::SpecialCoordinateSystem CoordinatesToUse, double radius);
};

} // namespace Mantid
} // namespace MDEvents

#endif /* MANTID_MDALGORITHMS_INTEGRATEPEAKSMD_H_ */
