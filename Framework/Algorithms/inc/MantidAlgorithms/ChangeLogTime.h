#ifndef CHANGELOGTIME_H
#define CHANGELOGTIME_H

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

class DLLExport ChangeLogTime : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CreateLogTimeCorrection", "ChangePulsetime", "ShiftLogTime"};
  }
  const std::string category() const override;
  /// Algorithm's summary
  const std::string summary() const override {
    return "Adds a constant to the times for the requested log.";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Mantid
} // namespace Algorithms
#endif // CHANGELOGTIME_H
