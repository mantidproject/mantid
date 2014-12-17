#ifndef CHANGELOGTIME_H
#define CHANGELOGTIME_H

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

class DLLExport ChangeLogTime : public API::Algorithm {
public:
  ChangeLogTime();
  ~ChangeLogTime();

  const std::string name() const;
  int version() const;
  const std::string category() const;
  /// Algorithm's summary
  virtual const std::string summary() const {
    return "Adds a constant to the times for the requested log.";
  }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
};

} // namespace Mantid
} // namespace Algorithms
#endif // CHANGELOGTIME_H
