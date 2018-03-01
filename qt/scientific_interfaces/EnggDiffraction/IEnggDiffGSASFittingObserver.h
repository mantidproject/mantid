#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASFITTINGOBSERVER_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASFITTINGOBSERVER_H_

#include "GSASIIRefineFitPeaksOutputProperties.h"

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffGSASFittingObserver {

public:
  virtual void notifyRefinementSuccessful(
      const GSASIIRefineFitPeaksOutputProperties &refinementResults) = 0;

  virtual void notifyRefinementFailed(const std::string &failureMessage) = 0;
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASFITTINGOBSERVER_H_
