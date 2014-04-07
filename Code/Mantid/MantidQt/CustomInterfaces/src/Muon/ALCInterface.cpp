#include "MantidQtCustomInterfaces/Muon/ALCInterface.h"

#include "MantidQtCustomInterfaces/Muon/ALCDataLoadingView.h"
#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingView.h"
#include "MantidQtCustomInterfaces/Muon/ALCPeakFittingView.h"

namespace MantidQt
{
namespace CustomInterfaces
{
  DECLARE_SUBWINDOW(ALCInterface);

  ALCInterface::ALCInterface(QWidget* parent)
    : UserSubWindow(parent), m_ui(), m_dataLoading(NULL), m_baselineModelling(NULL),
      m_peakFitting(NULL)
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

    auto peakFittingView = new ALCPeakFittingView(m_ui.peakFittingView);
    m_peakFitting = new ALCPeakFittingPresenter(peakFittingView);
    m_peakFitting->initialize();
  }

  void ALCInterface::nextStep()
  {
    int next = m_ui.stepView->currentIndex() + 1;

    if ( next < m_ui.stepView->count() )
    {
      auto nextWidget = m_ui.stepView->widget(next);

      if (nextWidget == m_ui.baselineModellingView)
      {
        m_baselineModelling->setData(m_dataLoading->loadedData());
      }
      else if (nextWidget == m_ui.peakFittingView)
      {
        m_peakFitting->setData(m_baselineModelling->correctedData());
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
