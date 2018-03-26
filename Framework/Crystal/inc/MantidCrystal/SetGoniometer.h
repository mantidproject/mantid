#ifndef MANTID_CRYSTAL_SETGONIOMETER_H_
#define MANTID_CRYSTAL_SETGONIOMETER_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Crystal {

/** Define the goniometer used in an experiment by giving the axes and
 *directions of rotations.
 *
 * @author Janik Zikovsky
 * @date 2011-05-27
 */
class DLLExport SetGoniometer : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SetGoniometer"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Define the goniometer motors used in an experiment by giving the "
           "axes and directions of rotations.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"GoniometerAnglesFromPhiRotation"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\Goniometer"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Mantid
} // namespace Crystal

#endif /* MANTID_CRYSTAL_SETGONIOMETER_H_ */
