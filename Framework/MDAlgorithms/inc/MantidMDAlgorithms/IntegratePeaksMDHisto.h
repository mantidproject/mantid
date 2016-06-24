#ifndef MANTID_MDALGORITHMS_INTEGRATEPEAKSMDHISTO_H_
#define MANTID_MDALGORITHMS_INTEGRATEPEAKSMHISTO_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDHistoWorkspace_fwd.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/MDHistoWorkspace.h"

namespace Mantid {
namespace MDAlgorithms {

/** Integrate single-crystal peaks in reciprocal-space.
 *
 * @author Vickie Lynch
 * @date 2016-06-23
 */
class DLLExport IntegratePeaksMDHisto : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "IntegratePeaksMD"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Integrate single-crystal peaks in reciprocal space, for "
           "MDHistoWorkspaces.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override { return "MDAlgorithms\\Peaks"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

};

} // namespace Mantid
} // namespace DataObjects

#endif /* MANTID_MDALGORITHMS_INTEGRATEPEAKSMDHISTO_H_ */
