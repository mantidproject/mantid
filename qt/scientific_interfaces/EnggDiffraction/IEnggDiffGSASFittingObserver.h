#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASFITTINGOBSERVER_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASFITTINGOBSERVER_H_

#include "GSASIIRefineFitPeaksOutputProperties.h"

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffGSASFittingObserver {

public:
  virtual ~IEnggDiffGSASFittingObserver() = default;

  /// Notify the observer that all refinements have terminated successfully
  virtual void notifyRefinementsComplete() = 0;

  /// Notify the observer that a single refinement has terminated successfully
  virtual void notifyRefinementSuccessful(
      const GSASIIRefineFitPeaksOutputProperties &refinementResults) = 0;

  /// Notify the observer that a refinement has failed
  virtual void notifyRefinementFailed(const std::string &failureMessage) = 0;

  /// Notify the observer that a single refinement was cancelled
  virtual void notifyRefinementCancelled() = 0;
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASFITTINGOBSERVER_H_
