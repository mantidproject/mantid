// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALCInterface.h"

#include "ALCBaselineModellingView.h"
#include "ALCDataLoadingView.h"
#include "ALCPeakFittingView.h"

#include "ALCBaselineModellingPresenter.h"
#include "ALCDataLoadingPresenter.h"
#include "ALCPeakFittingPresenter.h"

#include "ALCBaselineModellingModel.h"
#include "ALCPeakFittingModel.h"

#include "QInputDialog"
#include <QCloseEvent>

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/Logger.h"

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger logger("ALC Interface");

MatrixWorkspace_sptr getWorkspace(const std::string &workspaceName) {
  auto &ads = AnalysisDataService::Instance();
  if (ads.doesExist(workspaceName)) {
    return ads.retrieveWS<MatrixWorkspace>(workspaceName);
  } else {
    logger.warning("Workspace " + workspaceName + " was not found");
    return nullptr;
  }
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
DECLARE_SUBWINDOW(ALCInterface)

const QStringList ALCInterface::STEP_NAMES = {"Data loading", "Baseline modelling", "Peak fitting"};

// %1 - current step no., %2 - total no. of steps, %3 - current step label
const QString ALCInterface::LABEL_FORMAT = "Step %1/%2 - %3";

// Custom close event - only allows the window to close if loading is not taking
// place.
void ALCInterface::closeEvent(QCloseEvent *event) {
  // check if currently loading data
  if (m_dataLoading) {
    if (m_dataLoading->isLoading()) {
      m_dataLoading->cancelLoading();
      event->ignore();
    } else {
      event->accept();
    }
  } else {
    event->accept();
  }
}
// namespace CustomInterfaces
ALCInterface::ALCInterface(QWidget *parent)
    : UserSubWindow(parent), m_ui(), m_baselineModellingView(nullptr), m_peakFittingView(nullptr),
      m_dataLoading(nullptr), m_baselineModelling(nullptr), m_peakFitting(nullptr),
      m_baselineModellingModel(new ALCBaselineModellingModel()), m_peakFittingModel(new ALCPeakFittingModel()) {}

void ALCInterface::initLayout() {
  m_ui.setupUi(this);

  connect(m_ui.nextStep, SIGNAL(clicked()), SLOT(nextStep()));
  connect(m_ui.previousStep, SIGNAL(clicked()), SLOT(previousStep()));
  connect(m_ui.exportResults, SIGNAL(clicked()), SLOT(exportResults()));
  connect(m_ui.importResults, SIGNAL(clicked()), SLOT(importResults()));

  auto dataLoadingView = new ALCDataLoadingView(m_ui.dataLoadingView);
  m_dataLoading = new ALCDataLoadingPresenter(dataLoadingView);
  m_dataLoading->initialize();
  m_dataLoading->setParent(this);

  m_baselineModellingView = new ALCBaselineModellingView(m_ui.baselineModellingView);
  m_baselineModelling = new ALCBaselineModellingPresenter(m_baselineModellingView, m_baselineModellingModel);
  m_baselineModelling->initialize();

  m_peakFittingView = new ALCPeakFittingView(m_ui.peakFittingView);
  m_peakFitting = new ALCPeakFittingPresenter(m_peakFittingView, m_peakFittingModel);
  m_peakFitting->initialize();

  connect(m_dataLoading, SIGNAL(dataChanged()), SLOT(updateBaselineData()));
  connect(m_baselineModellingModel, SIGNAL(correctedDataChanged()), SLOT(updatePeakData()));

  assert(m_ui.stepView->count() == STEP_NAMES.count()); // Should have names for all steps

  switchStep(0); // We always start from the first step
}

void ALCInterface::updateBaselineData() {

  // Make sure we do have some data
  if (m_dataLoading->loadedData()) {

    // Send the data to BaselineModelling
    m_baselineModellingModel->setData(m_dataLoading->loadedData());

    // If we have a fitting function and a fitting range
    // we can update the baseline model
    if ((!m_baselineModellingView->function().isEmpty()) && (m_baselineModellingView->noOfSectionRows() > 0)) {

      // Fit the data
      m_baselineModellingView->emitFitRequested();
    }
  }
}

void ALCInterface::updatePeakData() {

  // Make sure we do have some data
  if (m_baselineModellingModel->correctedData()) {

    // Send the data to PeakFitting
    m_peakFittingModel->setData(m_baselineModellingModel->correctedData());

    // If we have a fitting function
    if (m_peakFittingView->function("")) {

      // Fit the data
      m_peakFittingView->emitFitRequested();
    }
  }
}

void ALCInterface::nextStep() {
  int next = m_ui.stepView->currentIndex() + 1;

  switchStep(next);
}

void ALCInterface::previousStep() {
  int previous = m_ui.stepView->currentIndex() - 1;

  switchStep(previous);
}

void ALCInterface::switchStep(int newStepIndex) {
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

  if (hasPrevStep) {
    m_ui.previousStep->setText("< " + STEP_NAMES[prevStepIndex]);
  }

  if (hasNextStep) {
    m_ui.nextStep->setText(STEP_NAMES[nextStepIndex] + " >");
  }

  m_ui.stepView->setCurrentIndex(newStepIndex);
}

void ALCInterface::exportResults() {

  bool ok;
  QString label = QInputDialog::getText(this, "Results label", "Label to assign to the results: ", QLineEdit::Normal,
                                        "ALCResults", &ok);

  if (!ok) // Cancelled
  {
    return;
  }

  std::string groupName = label.toStdString();

  using namespace Mantid::API;

  std::map<std::string, Workspace_sptr> results;

  if (const auto loadedData = m_dataLoading->exportWorkspace())
    results["Loaded_Data"] = loadedData->clone();
  if (const auto baseline = m_baselineModellingModel->exportWorkspace())
    results["Baseline_Workspace"] = baseline->clone();
  if (const auto baselineSections = m_baselineModellingModel->exportSections())
    results["Baseline_Sections"] = baselineSections->clone();
  if (const auto baselineModel = m_baselineModellingModel->exportModel())
    results["Baseline_Model"] = baselineModel->clone();
  if (const auto peaksWorkspace = m_peakFittingModel->exportWorkspace())
    results["Peaks_Workspace"] = peaksWorkspace->clone();
  if (const auto peaksResults = m_peakFittingModel->exportFittedPeaks())
    results["Peaks_FitResults"] = peaksResults->clone();

  // Check if any of the above is not empty
  bool nothingToExport = true;
  for (auto &result : results) {

    if (result.second) {
      nothingToExport = false;
      break;
    }
  }

  // There is something to export
  if (!nothingToExport) {

    // Add output group to the ADS
    AnalysisDataService::Instance().addOrReplace(groupName, std::make_shared<WorkspaceGroup>());

    for (auto &result : results) {
      if (result.second) {
        std::string wsName = groupName + "_" + result.first;
        AnalysisDataService::Instance().addOrReplace(wsName, result.second);
        AnalysisDataService::Instance().addToGroup(groupName, wsName);
      }
    }
  } else {
    // Nothing to export, show error message
    QMessageBox::critical(this, "Error", "Nothing to export");
  }
}

void ALCInterface::importResults() {
  bool okClicked;
  const auto groupName =
      QInputDialog::getText(this, "Results label", "Label to assign to the results: ", QLineEdit::Normal, "ALCResults",
                            &okClicked)
          .toStdString();

  if (!okClicked) {
    return;
  } else if (!AnalysisDataService::Instance().doesExist(groupName)) {
    QMessageBox::critical(this, "Error", "Workspace " + QString::fromStdString(groupName) + " could not be found.");
  }

  importLoadedData(groupName + "_Loaded_Data");
  importBaselineData(groupName + "_Baseline_Workspace");
  importPeakData(groupName + "_Peaks_Workspace");
}

void ALCInterface::importLoadedData(const std::string &workspaceName) {
  if (const auto dataWS = getWorkspace(workspaceName)) {
    if (dataWS->getNumberHistograms() != 1) {
      logger.warning("Workspace " + workspaceName + " must contain one spectrum only.");
    } else {
      m_dataLoading->setData(dataWS);
    }
  }
}

void ALCInterface::importBaselineData(const std::string &workspaceName) {
  if (const auto baselineWS = getWorkspace(workspaceName)) {
    if (baselineWS->getNumberHistograms() != 3) {
      logger.warning("Workspace " + workspaceName + " must contain three spectra.");
    } else {
      m_baselineModellingModel->setData(baselineWS);
      m_baselineModellingModel->setCorrectedData(baselineWS);
    }
  }
}

void ALCInterface::importPeakData(const std::string &workspaceName) {
  if (const auto peaksWS = getWorkspace(workspaceName)) {
    if (peaksWS->getNumberHistograms() < 3) {
      logger.warning("Workspace " + workspaceName + " must contain at least three spectra.");
    } else {
      m_peakFittingModel->setData(peaksWS);
    }
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
