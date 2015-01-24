#ifndef MANTID_ALGORITHMS_CHANGEPULSETIME_H_
#define MANTID_ALGORITHMS_CHANGEPULSETIME_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/** ChangePulsetime : TODO: DESCRIPTION
 *
 * @author
 * @date 2011-03-31 09:31:55.674594
 */
class DLLExport ChangePulsetime : public API::Algorithm {
public:
  ChangePulsetime();
  ~ChangePulsetime();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "ChangePulsetime"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Adds a constant time value, in seconds, to the pulse time of "
           "events in an EventWorkspace. ";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Events;Transforms\\Axes";
  }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
};

} // namespace Mantid
} // namespace Algorithms

#endif /* MANTID_ALGORITHMS_CHANGEPULSETIME_H_ */
