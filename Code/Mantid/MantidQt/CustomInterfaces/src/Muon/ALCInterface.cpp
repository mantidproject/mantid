#include "MantidQtCustomInterfaces/Muon/ALCInterface.h"

#include "MantidQtCustomInterfaces/Muon/ALCDataLoadingView.h"
#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingView.h"

namespace MantidQt
{
namespace CustomInterfaces
{
  DECLARE_SUBWINDOW(ALCInterface);

  ALCInterface::ALCInterface(QWidget* parent)
    : UserSubWindow(parent), m_ui()
  {}

  void ALCInterface::initLayout()
  {
    m_ui.setupUi(this);

    connect(m_ui.nextStep, SIGNAL(pressed()), this, SLOT(nextStep()));
    connect(m_ui.previousStep, SIGNAL(pressed()), this, SLOT(previousStep()));

    auto view1 = new ALCDataLoadingView(m_ui.dataLoadingView);

    auto ws = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>("ALCWorkspace");
    auto view = new ALCBaselineModellingView(m_ui.baselineModellingView, ws);
    view->initialize();
  }

  void ALCInterface::nextStep()
  {
    int next = m_ui.stepView->currentIndex() + 1;

    if ( next < m_ui.stepView->count() )
    {
      m_ui.stepView->setCurrentIndex(next);
    }
  }

  void ALCInterface::previousStep()
  {
    int previous = m_ui.stepView->currentIndex() - 1;

    if ( previous >= 0 )
    {
      m_ui.stepView->setCurrentIndex(previous);
    }
  }

} // namespace CustomInterfaces
} // namespace MantidQt
