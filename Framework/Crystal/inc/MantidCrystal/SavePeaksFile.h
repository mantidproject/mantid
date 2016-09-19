#ifndef MANTID_CRYSTAL_SAVEPEAKSFILE_H_
#define MANTID_CRYSTAL_SAVEPEAKSFILE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Crystal {

/** SavePeaksFile : Save a PeaksWorkspace to a .peaks text-format file.
 *
 * @author Janik Zikovsky
 * @date 2011-03-18 14:18:59.523067
 */
class DLLExport SavePeaksFile : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SavePeaksFile"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Save a PeaksWorkspace to a .peaks text-format file.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Mantid
} // namespace Crystal

#endif /* MANTID_CRYSTAL_SAVEPEAKSFILE_H_ */
