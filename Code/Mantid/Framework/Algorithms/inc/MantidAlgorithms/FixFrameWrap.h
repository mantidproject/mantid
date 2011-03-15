#ifndef MANTID_ALGORITHMS_FIXFRAMEWRAP_H_
#define MANTID_ALGORITHMS_FIXFRAMEWRAP_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace Algorithms
{
class DLLExport FixFrameWrap: public API::Algorithm
{
public:
  FixFrameWrap();
  virtual ~FixFrameWrap();
  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;
private:
  virtual void initDocs();
  void init();
  void exec();
  void cleanup();
  void execEvent();
  double calculateFlightpath(const int& spectrum, const double& L1, bool& isMonitor) const;

  /// Shared pointer to a mutable input workspace
  API::MatrixWorkspace_sptr matrixInputW;

  /// Shared pointer to the event workspace
  DataObjects::EventWorkspace_sptr eventInputW;

  /// Primary flight path
  double L1;
};
} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_FIXFRAMEWRAP_H_ */
