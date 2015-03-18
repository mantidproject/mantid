#ifndef MANTID_MDEVENTS_CREATEMDWORKSPACE_H_
#define MANTID_MDEVENTS_CREATEMDWORKSPACE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/BoxControllerSettingsAlgorithm.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"

namespace Mantid {
namespace MDAlgorithms {

/** CreateMDWorkspace :
 *
 * Algorithm to create an empty MDEventWorkspace with a given number of
 *dimensions.
 *
 *
 * @author Janik Zikovsky
 * @date 2011-02-25 11:54:52.003137
 */
class DLLExport CreateMDWorkspace
    : public MDEvents::BoxControllerSettingsAlgorithm {
public:
  CreateMDWorkspace();
  ~CreateMDWorkspace();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "CreateMDWorkspace"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Creates an empty MDEventWorkspace with a given number of "
           "dimensions.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MDAlgorithms"; }

private:
  void init();
  void exec();

  template <typename MDE, size_t nd>
  void finish(typename MDEvents::MDEventWorkspace<MDE, nd>::sptr ws);
};

} // namespace Mantid
} // namespace MDEvents

#endif /* MANTID_MDEVENTS_CREATEMDWORKSPACE_H_ */
