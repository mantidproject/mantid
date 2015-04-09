#include "MantidQtCustomInterfaces/Muon/ALCInterface.h"

#include "MantidQtCustomInterfaces/Muon/ALCDataLoadingView.h"
#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingView.h"
#include "MantidQtCustomInterfaces/Muon/ALCPeakFittingView.h"

#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingModel.h"
#include "MantidQtCustomInterfaces/Muon/ALCPeakFittingModel.h"

#include "QInputDialog"

#include "MantidAPI/WorkspaceGroup.h"

namespace MantidQt
{
namespace CustomInterfaces
{
  DECLARE_SUBWINDOW(ALCInterface)

  const QStringList ALCInterface::STEP_NAMES =
      QStringList() << "Data loading" << "Baseline modelling" << "Peak fitting";

  // %1 - current step no., %2 - total no. of steps, %3 - current step label
  const QString ALCInterface::LABEL_FORMAT = "Step %1/%2 - %3";

  ALCInterface::ALCInterface(QWidget* parent)
    : UserSubWindow(parent), m_ui(),
      m_dataLoading(NULL), m_baselineModelling(NULL), m_peakFitting(NULL),
      m_baselineModellingModel(new ALCBaselineModellingModel()),
      m_peakFittingModel(new ALCPeakFittingModel())
  {}

  void ALCInterface::initLayout()
  {
    m_ui.setupUi(this);

    connect(m_ui.nextStep, SIGNAL(clicked()), SLOT(nextStep()));
    connect(m_ui.previousStep, SIGNAL(clicked()), SLOT(previousStep()));
    connect(m_ui.exportResults, SIGNAL(clicked()), SLOT(exportResults()));

    auto dataLoadingView = new ALCDataLoadingView(m_ui.dataLoadingView);
    m_dataLoading = new ALCDataLoadingPresenter(dataLoadingView);
    m_dataLoading->initialize();

    auto baselineModellingView = new ALCBaselineModellingView(m_ui.baselineModellingView);
    m_baselineModelling = new ALCBaselineModellingPresenter(baselineModellingView, m_baselineModellingModel);
    m_baselineModelling->initialize();

    auto peakFittingView = new ALCPeakFittingView(m_ui.peakFittingView);
    m_peakFitting = new ALCPeakFittingPresenter(peakFittingView, m_peakFittingModel);
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
      if (m_dataLoading->loadedData())
      {
        m_baselineModellingModel->setData(m_dataLoading->loadedData());
      }
      else
      {
        QMessageBox::critical(this, "Error", "Please load some data first");
        return;
      }
    }
    if (nextWidget == m_ui.peakFittingView)
    {
      if (m_baselineModellingModel->correctedData())
      {
        m_peakFittingModel->setData(m_baselineModellingModel->correctedData());
      }
      else
      {
        QMessageBox::critical(this, "Error", "Please fit a baseline first");
        return;
      }
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

    bool hasNextStep = (nextStepIndex < m_ui.stepView->count());
    bool hasPrevStep = (prevStepIndex >= 0);

    m_ui.previousStep->setVisible(hasPrevStep);

    // On last step - hide next step button, but show "Export results..."
    m_ui.nextStep->setVisible(hasNextStep);

    if (hasPrevStep)
    {
      m_ui.previousStep->setText("< " + STEP_NAMES[prevStepIndex]);
    }

    if (hasNextStep)
    {
      m_ui.nextStep->setText(STEP_NAMES[nextStepIndex] + " >");
    }

    m_ui.stepView->setCurrentIndex(newStepIndex);
  }

  void ALCInterface::exportResults()
  {

    bool ok;
    QString label = QInputDialog::getText(this, "Results label", "Label to assign to the results: ",
                                         QLineEdit::Normal, "ALCResults", &ok);

    if (!ok) // Cancelled
    {
      return;
    }

    std::string groupName = label.toStdString();

    using namespace Mantid::API;

    std::map<std::string, Workspace_sptr> results;

    results["Baseline_Workspace"] = m_baselineModellingModel->exportWorkspace();
    results["Baseline_Sections"] = m_baselineModellingModel->exportSections();
    results["Baseline_Model"] = m_baselineModellingModel->exportModel();

    results["Peaks_Workspace"] = m_peakFittingModel->exportWorkspace();
    results["Peaks_FitResults"] = m_peakFittingModel->exportFittedPeaks();

    AnalysisDataService::Instance().addOrReplace(groupName, boost::make_shared<WorkspaceGroup>());

    for(auto it = results.begin(); it != results.end(); ++it)
    {
      std::string wsName = groupName + "_" + it->first;
      AnalysisDataService::Instance().addOrReplace(wsName, it->second);
      AnalysisDataService::Instance().addToGroup(groupName, wsName);
    }
  }

} // namespace CustomInterfaces
} // namespace MantidQt
