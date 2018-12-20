#ifndef MANTID_MDALGORITHMS_ONESTEPMDEW_H_
#define MANTID_MDALGORITHMS_ONESTEPMDEW_H_

#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace MDAlgorithms {

/** OneStepMDEW : Assumes elastic diffration. Load Event data and immediately
 *converts to MDEvent with Lorentz correction applied.
 *
 * @author
 * @date 2011-04-06 10:19:10.284945
 */
class DLLExport OneStepMDEW : public API::Algorithm, API::DeprecatedAlgorithm {
public:
  OneStepMDEW();

  /// Algorithm's name for identification
  const std::string name() const override { return "OneStepMDEW"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Create a MDEventWorkspace in one step from a EventNexus file. For "
           "use by Paraview loader.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override {
    return "MDAlgorithms\\Creation";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_ONESTEPMDEW_H_ */
