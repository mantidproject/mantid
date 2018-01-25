#ifndef MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETVIEW_H_

#include "MantidAPI/MatrixWorkspace.h"

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffMultiRunFittingWidgetView {
public:
  virtual ~IEnggDiffMultiRunFittingWidgetView() = default;

  virtual void addFittedPeaks(const int runNumber, const size_t bank,
                              const Mantid::API::MatrixWorkspace_sptr ws) = 0;

  virtual void addFocusedRun(const int runNumber, const size_t bank,
                             const Mantid::API::MatrixWorkspace_sptr ws) = 0;

  virtual void getFittedPeaks(const int runNumber, const size_t bank) const = 0;

  virtual Mantid::API::MatrixWorkspace_sptr
  getFocusedRun(const int runNumber, const size_t bank) const = 0;
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETVIEW_H_
