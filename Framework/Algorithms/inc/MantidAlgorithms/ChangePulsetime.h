#ifndef MANTID_ALGORITHMS_CHANGEPULSETIME_H_
#define MANTID_ALGORITHMS_CHANGEPULSETIME_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** ChangePulsetime : TODO: DESCRIPTION
 *
 * @author
 * @date 2011-03-31 09:31:55.674594
 */
class DLLExport ChangePulsetime : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "ChangePulsetime"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Adds a constant time value, in seconds, to the pulse time of "
           "events in an EventWorkspace. ";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"CreateLogTimeCorrection", "CalculateCountRate"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Events;Transforms\\Axes";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CHANGEPULSETIME_H_ */
