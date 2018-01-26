#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTER_H_

#include "DllConfig.h"
#include "IEnggDiffMultiRunFittingWidgetModel.h"
#include "IEnggDiffMultiRunFittingWidgetPresenter.h"
#include "IEnggDiffMultiRunFittingWidgetView.h"

#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffMultiRunFittingWidgetPresenter
    : public IEnggDiffMultiRunFittingWidgetPresenter {

public:
  EnggDiffMultiRunFittingWidgetPresenter(
      std::unique_ptr<IEnggDiffMultiRunFittingWidgetModel> model,
      IEnggDiffMultiRunFittingWidgetView *view);

  ~EnggDiffMultiRunFittingWidgetPresenter() override;

  void
  notify(IEnggDiffMultiRunFittingWidgetPresenter::Notification notif) override;

private:
  void processAddFittedPeaks();
  void processAddFocusedRun();
  void processGetFittedPeaks();
  void processGetFocusedRun();
  void processShutDown();
  void processStart();

  std::unique_ptr<IEnggDiffMultiRunFittingWidgetModel> m_model;

  IEnggDiffMultiRunFittingWidgetView *m_view;

  bool m_viewHasClosed;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTER_H_
