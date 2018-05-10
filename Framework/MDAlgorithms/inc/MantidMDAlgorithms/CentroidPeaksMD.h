#ifndef MANTID_MDALGORITHMS_CENTROIDPEAKSMD_H_
#define MANTID_MDALGORITHMS_CENTROIDPEAKSMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace MDAlgorithms {

/** Find the centroid of single-crystal peaks in a MDEventWorkspace, in order to
 *refine their positions.
 *
 * @author Janik Zikovsky
 * @date 2011-06-01
 */
class DLLExport CentroidPeaksMD : public API::Algorithm,
                                  public API::DeprecatedAlgorithm {
public:
  /// Constructor
  CentroidPeaksMD();

  /// Algorithm's name for identification
  const std::string name() const override { return "CentroidPeaksMD"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Find the centroid of single-crystal peaks in a MDEventWorkspace, "
           "in order to refine their positions.";
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

  template <typename MDE, size_t nd>
  void integrate(typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

  /// Input MDEventWorkspace
  Mantid::API::IMDEventWorkspace_sptr inWS;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CENTROIDPEAKSMD_H_ */
