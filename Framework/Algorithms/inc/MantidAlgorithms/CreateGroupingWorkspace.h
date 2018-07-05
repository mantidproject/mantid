#ifndef MANTID_ALGORITHMS_CREATEGROUPINGWORKSPACE_H_
#define MANTID_ALGORITHMS_CREATEGROUPINGWORKSPACE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

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
  /// Algorithm's name for identification
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Creates a new GroupingWorkspace using an instrument from one of: "
           "an input workspace, an instrument name, or an instrument IDF "
           "file.\nOptionally uses bank names to create the groups.";
  }

  /// Algorithm's version for identification
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"DiffractionFocussing", "LoadCalFile"};
  }
  /// Algorithm's category for identification
  const std::string category() const override;

private:
  /// Initialise the properties
  void init() override;
  /// Cross-check properties with each other @see IAlgorithm::validateInputs
  std::map<std::string, std::string> validateInputs() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CREATEGROUPINGWORKSPACE_H_ */
