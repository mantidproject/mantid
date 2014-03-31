#include "MantidQtCustomInterfaces/Muon/ALCInterface.h"

#include "MantidQtCustomInterfaces/Muon/ALCDataLoadingView.h"
#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingView.h"

namespace MantidQt
{
namespace CustomInterfaces
{
  DECLARE_SUBWINDOW(ALCInterface);

  ALCInterface::ALCInterface(QWidget* parent)
    : UserSubWindow(parent), m_ui(), m_dataLoading(NULL), m_baselineModelling(NULL)
  {}

  void ALCInterface::initLayout()
  {
    m_ui.setupUi(this);

    connect(m_ui.nextStep, SIGNAL(pressed()), this, SLOT(nextStep()));
    connect(m_ui.previousStep, SIGNAL(pressed()), this, SLOT(previousStep()));

    auto dataLoadingView = new ALCDataLoadingView(m_ui.dataLoadingView);
    m_dataLoading = new ALCDataLoadingPresenter(dataLoadingView);
    m_dataLoading->initialize();

    auto baselineModellingView = new ALCBaselineModellingView(m_ui.baselineModellingView);
    m_baselineModelling = new ALCBaselineModellingPresenter(baselineModellingView);
    m_baselineModelling->initialize();
  }

  void ALCInterface::nextStep()
  {
    int next = m_ui.stepView->currentIndex() + 1;

    if ( next < m_ui.stepView->count() )
    {
      if (m_ui.stepView->widget(next) == m_ui.baselineModellingView)
      {
        m_baselineModelling->setData(m_dataLoading->loadedData());
      }

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
