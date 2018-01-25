#ifndef MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETMODEL_H_
#define MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETMODEL_H_

#include "MantidAPI/MatrixWorkspace.h"

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffMultiRunFittingWidgetModel {
public:
  virtual ~IEnggDiffMultiRunFittingWidgetModel() = default;

  virtual void addFittedPeaks(const int runNumber, const size_t bank,
                              const Mantid::API::MatrixWorkspace_sptr ws) = 0;

  virtual void addFocusedRun(const int runNumber, const size_t bank,
                             const Mantid::API::MatrixWorkspace_sptr ws) = 0;

  virtual void getFittedPeaks(const int runNumber, const size_t bank) const = 0;

  virtual Mantid::API::MatrixWorkspace_sptr
  getFocusedRun(const int runNumber, const size_t bank) const = 0;

};

} // namespace MantidQt
} // namespace CustomInterfaces

#endif // MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETMODEL_H_
