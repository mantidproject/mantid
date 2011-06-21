#ifndef CHANGELOGTIME_H
#define CHANGELOGTIME_H

#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{

class MANTID_API_DLL ChangeLogTime : public API::Algorithm
{
public:
    ChangeLogTime();
    ~ChangeLogTime();

    const std::string name() const;
    int version() const;
    const std::string category() const;

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
};

} // namespace Mantid
} // namespace Algorithms
#endif // CHANGELOGTIME_H
