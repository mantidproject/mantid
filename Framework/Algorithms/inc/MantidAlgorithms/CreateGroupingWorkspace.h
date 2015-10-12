#ifndef MANTID_ALGORITHMS_CREATEGROUPINGWORKSPACE_H_
#define MANTID_ALGORITHMS_CREATEGROUPINGWORKSPACE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/** Creates a new GroupingWorkspace using an instrument from one of:
 *  an input workspace,
 *  an instrument name,
 *  or an instrument IDF file.
 *
 *  Optionally uses bank names to create the groups.
 */
class DLLExport CreateGroupingWorkspace : public API::Algorithm {
public:
  CreateGroupingWorkspace();
  ~CreateGroupingWorkspace();

  /// Algorithm's name for identification
  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Creates a new GroupingWorkspace using an instrument from one of: "
           "an input workspace, an instrument name, or an instrument IDF "
           "file.\nOptionally uses bank names to create the groups.";
  }

  /// Algorithm's version for identification
  virtual int version() const;
  /// Algorithm's category for identification
  virtual const std::string category() const;

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
};

} // namespace Mantid
} // namespace Algorithms

#endif /* MANTID_ALGORITHMS_CREATEGROUPINGWORKSPACE_H_ */
