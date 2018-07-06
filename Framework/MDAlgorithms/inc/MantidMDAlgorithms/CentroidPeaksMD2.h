#ifndef MANTID_MDALGORITHMS_CENTROIDPEAKSMD2_H_
#define MANTID_MDALGORITHMS_CENTROIDPEAKSMD2_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/MDEventWorkspace.h"

namespace Mantid {
namespace MDAlgorithms {

/** Find the centroid of single-crystal peaks in a MDEventWorkspace, in order to
 *refine their positions.
 *
 * @author Janik Zikovsky
 * @date 2011-06-01
 */
class DLLExport CentroidPeaksMD2 : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "CentroidPeaksMD"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Find the centroid of single-crystal peaks in a MDEventWorkspace, "
           "in order to refine their positions.";
  }

  /// Algorithm's version for identification
  int version() const override { return 2; };
  const std::vector<std::string> seeAlso() const override {
    return {"IntegratePeaksMD", "CentroidPeaks"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "MDAlgorithms\\Peaks"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  template <typename MDE, size_t nd>
  void integrate(typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

  /// Input MDEventWorkspace
  Mantid::API::IMDEventWorkspace_sptr inWS;
};

} // namespace Mantid
} // namespace DataObjects

#endif /* MANTID_MDALGORITHMS_CENTROIDPEAKSMD2_H_ */
