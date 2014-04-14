#include "MantidQtCustomInterfaces/Muon/ALCInterface.h"

#include "MantidQtCustomInterfaces/Muon/ALCDataLoadingView.h"
#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingView.h"
#include "MantidQtCustomInterfaces/Muon/ALCPeakFittingView.h"

#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingModel.h"

namespace MantidQt
{
namespace CustomInterfaces
{
  DECLARE_SUBWINDOW(ALCInterface);

  const QStringList ALCInterface::STEP_NAMES =
      QStringList() << "Data loading" << "Baseline modelling" << "Peak fitting";

  // %1 - current step no., %2 - total no. of steps, %3 - current step label
  const QString ALCInterface::LABEL_FORMAT = "Step %1/%2 - %3";

  ALCInterface::ALCInterface(QWidget* parent)
    : UserSubWindow(parent), m_ui(), m_dataLoading(NULL), m_baselineModelling(NULL),
      m_peakFitting(NULL)
  {}

  void ALCInterface::initLayout()
  {
    m_ui.setupUi(this);

    connect(m_ui.nextStep, SIGNAL(clicked()), this, SLOT(nextStep()));
    connect(m_ui.previousStep, SIGNAL(clicked()), this, SLOT(previousStep()));

    auto dataLoadingView = new ALCDataLoadingView(m_ui.dataLoadingView);
    m_dataLoading = new ALCDataLoadingPresenter(dataLoadingView);
    m_dataLoading->initialize();

    auto baselineModellingView = new ALCBaselineModellingView(m_ui.baselineModellingView);
    auto baselineModellingModel = new ALCBaselineModellingModel();
    m_baselineModelling = new ALCBaselineModellingPresenter(baselineModellingView, baselineModellingModel);
    m_baselineModelling->initialize();

    auto peakFittingView = new ALCPeakFittingView(m_ui.peakFittingView);
    m_peakFitting = new ALCPeakFittingPresenter(peakFittingView);
    m_peakFitting->initialize();

    assert(m_ui.stepView->count() == STEP_NAMES.count()); // Should have names for all steps

    switchStep(0); // We always start from the first step
  }

  void ALCInterface::nextStep()
  {
    int next = m_ui.stepView->currentIndex() + 1;

    auto nextWidget = m_ui.stepView->widget(next);
    assert(nextWidget);

    if (nextWidget == m_ui.baselineModellingView)
    {
      m_baselineModelling->setData(m_dataLoading->loadedData());
    }
    if (nextWidget == m_ui.peakFittingView)
    {
      m_peakFitting->setData(m_baselineModelling->model().correctedData());
    }

    switchStep(next);
  }

  void ALCInterface::previousStep()
  {
    int previous = m_ui.stepView->currentIndex() - 1;

    switchStep(previous);
  }

  void ALCInterface::switchStep(int newStepIndex)
  {
    // Should be disallowed by disabling buttons
    assert(newStepIndex >= 0);
    assert(newStepIndex < m_ui.stepView->count());

    m_ui.label->setText(LABEL_FORMAT.arg(newStepIndex + 1).arg(STEP_NAMES.count()).arg(STEP_NAMES[newStepIndex]));

    int nextStepIndex = newStepIndex + 1;
    int prevStepIndex = newStepIndex - 1;

    bool nextStepVisible = (nextStepIndex < m_ui.stepView->count());
    bool prevStepVisible = (prevStepIndex >= 0);

    m_ui.nextStep->setVisible(nextStepVisible);
    m_ui.previousStep->setVisible(prevStepVisible);

    if (nextStepVisible)
      m_ui.nextStep->setText(STEP_NAMES[nextStepIndex] + " >");

    if (prevStepVisible)
      m_ui.previousStep->setText("< " + STEP_NAMES[prevStepIndex]);

    m_ui.stepView->setCurrentIndex(newStepIndex);
  }

} // namespace CustomInterfaces
} // namespace MantidQt
