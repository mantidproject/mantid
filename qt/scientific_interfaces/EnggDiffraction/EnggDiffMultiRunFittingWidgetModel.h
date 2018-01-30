#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODEL_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODEL_H_

#include "DllConfig.h"
#include "IEnggDiffMultiRunFittingWidgetModel.h"
#include "RunMap.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffMultiRunFittingWidgetModel
    : public IEnggDiffMultiRunFittingWidgetModel {

public:
  void addFittedPeaks(const int runNumber, const size_t bank,
                      const Mantid::API::MatrixWorkspace_sptr ws) override;

  void addFocusedRun(const int runNumber, const size_t bank,
                     const Mantid::API::MatrixWorkspace_sptr ws) override;

  std::vector<std::pair<int, size_t>> getAllWorkspaceLabels() const override;

  boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFittedPeaks(const int runNumber, const size_t bank) const override;

  boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFocusedRun(const int runNumber, const size_t bank) const override;

private:
  static constexpr size_t MAX_BANKS = 2;

  RunMap<MAX_BANKS, Mantid::API::MatrixWorkspace_sptr> m_fittedPeaksMap;
  RunMap<MAX_BANKS, Mantid::API::MatrixWorkspace_sptr> m_focusedRunMap;
};

} // namespace MantidQt
} // namespace CustomInterfaces

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODEL_H_
