#ifndef MANTID_ALGORITHMS_CREATEPEAKSWORKSPACE_H_
#define MANTID_ALGORITHMS_CREATEPEAKSWORKSPACE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/** Create an empty PeaksWorkspace.
 *
 * @author Janik Zikovsky
 * @date 2011-04-26 08:49:10.540441
 */
class DLLExport CreatePeaksWorkspace : public API::Algorithm {
public:
  CreatePeaksWorkspace();
  ~CreatePeaksWorkspace();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "CreatePeaksWorkspace"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Create an empty PeaksWorkspace.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Crystal;Utility\\Workspaces";
  }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
};

} // namespace Mantid
} // namespace Algorithms

#endif /* MANTID_ALGORITHMS_CREATEPEAKSWORKSPACE_H_ */
