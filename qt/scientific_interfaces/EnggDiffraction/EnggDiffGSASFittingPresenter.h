#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFGSASFITTINGPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFGSASFITTINGPRESENTER_H_

#include "DllConfig.h"
#include "IEnggDiffGSASFittingModel.h"
#include "IEnggDiffGSASFittingPresenter.h"
#include "IEnggDiffGSASFittingView.h"

#include <QObject>

#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

// needs to be dll-exported for the tests
class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffGSASFittingPresenter
    : public QObject,
      public IEnggDiffGSASFittingPresenter {
  // Q_OBJECT for 'connect' with thread/worker
  Q_OBJECT

public:
  EnggDiffGSASFittingPresenter(std::unique_ptr<IEnggDiffGSASFittingModel> model,
                               std::unique_ptr<IEnggDiffGSASFittingView> view);

  ~EnggDiffGSASFittingPresenter() override;

  void notify(IEnggDiffGSASFittingPresenter::Notification notif) override;

private:
  void processShutDown();
  void processStart();

  std::unique_ptr<IEnggDiffGSASFittingModel> m_model;

  std::unique_ptr<IEnggDiffGSASFittingView> m_view;

  bool m_viewHasClosed;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif
