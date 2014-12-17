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
  SetGoniometer();
  ~SetGoniometer();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "SetGoniometer"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Define the goniometer motors used in an experiment by giving the "
           "axes and directions of rotations.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Crystal"; }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
};

} // namespace Mantid
} // namespace Crystal

#endif /* MANTID_CRYSTAL_SETGONIOMETER_H_ */
