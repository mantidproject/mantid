#ifndef MANTID_CRYSTAL_LOADISAWUB_H_
#define MANTID_CRYSTAL_LOADISAWUB_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Crystal {

/** Algorithm to load an ISAW-style ASCII UB matrix and lattice
 * parameters file, and place its information into
 * a workspace.
 *
 * @author Janik Zikovsky
 * @date 2011-05-25
 */
class DLLExport LoadIsawUB : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "LoadIsawUB"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load an ISAW-style ASCII UB matrix and lattice parameters file, "
           "and place its information into a workspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"SaveIsawUB"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Crystal\\DataHandling;DataHandling\\Isaw";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Mantid
} // namespace Crystal

#endif /* MANTID_CRYSTAL_LOADISAWUB_H_ */
