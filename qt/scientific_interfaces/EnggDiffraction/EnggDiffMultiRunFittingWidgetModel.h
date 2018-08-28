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
  void addFittedPeaks(const RunLabel &runLabel,
                      const Mantid::API::MatrixWorkspace_sptr ws) override;

  void addFocusedRun(const RunLabel &runLabel,
                     const Mantid::API::MatrixWorkspace_sptr ws) override;

  std::vector<RunLabel> getAllWorkspaceLabels() const override;

  boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFittedPeaks(const RunLabel &runLabel) const override;

  boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFocusedRun(const RunLabel &runLabel) const override;

  bool hasFittedPeaksForRun(const RunLabel &runLabel) const override;

  void removeRun(const RunLabel &runLabel) override;

private:
  static constexpr size_t MAX_BANKS = 3;

  RunMap<MAX_BANKS, Mantid::API::MatrixWorkspace_sptr> m_fittedPeaksMap;
  RunMap<MAX_BANKS, Mantid::API::MatrixWorkspace_sptr> m_focusedRunMap;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODEL_H_
