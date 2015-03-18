#ifndef MANTID_MDEVENTS_ONESTEPMDEW_H_
#define MANTID_MDEVENTS_ONESTEPMDEW_H_

#include "MantidKernel/System.h"
#include "MantidAPI/DeprecatedAlgorithm.h"

namespace Mantid {
namespace MDEvents {

/** OneStepMDEW : Assumes elastic diffration. Load Event data and immediately
 *converts to MDEvent with Lorentz correction applied.
 *
 * @author
 * @date 2011-04-06 10:19:10.284945
 */
class DLLExport OneStepMDEW : public API::Algorithm, API::DeprecatedAlgorithm {
public:
  OneStepMDEW();
  ~OneStepMDEW();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "OneStepMDEW"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Create a MDEventWorkspace in one step from a EventNexus file. For "
           "use by Paraview loader.";
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
};

} // namespace Mantid
} // namespace MDEvents

#endif /* MANTID_MDEVENTS_ONESTEPMDEW_H_ */
