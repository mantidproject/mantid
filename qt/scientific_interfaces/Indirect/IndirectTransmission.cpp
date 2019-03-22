// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectTransmission.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <QFileInfo>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
IndirectTransmission::IndirectTransmission(IndirectDataReduction *idrUI,
                                           QWidget *parent)
    : IndirectDataReductionTab(idrUI, parent) {
  m_uiForm.setupUi(parent);

  connect(this, SIGNAL(newInstrumentConfiguration()), this,
          SLOT(instrumentSet()));

  // Update the preview plot when the algorithm is complete
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(transAlgDone(bool)));

  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));

  connect(this,
          SIGNAL(updateRunButton(bool, std::string const &, QString const &,
                                 QString const &)),
          this,
          SLOT(updateRunButton(bool, std::string const &, QString const &,
                               QString const &)));
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
IndirectTransmission::~IndirectTransmission() {}

void IndirectTransmission::setup() {}

void IndirectTransmission::run() {
  QString sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
  QString canWsName = m_uiForm.dsCanInput->getCurrentDataName();
  QString outWsName = sampleWsName + "_transmission";

  IAlgorithm_sptr transAlg =
      AlgorithmManager::Instance().create("IndirectTransmissionMonitor", -1);
  transAlg->initialize();

  transAlg->setProperty("SampleWorkspace", sampleWsName.toStdString());
  transAlg->setProperty("CanWorkspace", canWsName.toStdString());
  transAlg->setProperty("OutputWorkspace", outWsName.toStdString());

  m_batchAlgoRunner->addAlgorithm(transAlg);
  m_batchAlgoRunner->executeBatchAsync();
}

bool IndirectTransmission::validate() {
  // Check if we have an appropriate instrument
  QString currentInst = getInstrumentName();
  if (currentInst != "IRIS" && currentInst != "OSIRIS")
    return false;

  // Check for an invalid sample input
  if (!m_uiForm.dsSampleInput->isValid())
    return false;

  // Check for an invalid can input
  if (!m_uiForm.dsCanInput->isValid())
    return false;

  return true;
}

void IndirectTransmission::transAlgDone(bool error) {
  if (error)
    return;

  QString sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
  QString outWsName = sampleWsName + "_transmission";

  WorkspaceGroup_sptr resultWsGroup =
      AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
          outWsName.toStdString());

  // Do plotting
  m_uiForm.ppPlot->clear();
  m_uiForm.ppPlot->addSpectrum("Can", sampleWsName + "_Can", 0, Qt::black);
  m_uiForm.ppPlot->addSpectrum("Sample", sampleWsName + "_Sam", 0, Qt::red);
  m_uiForm.ppPlot->addSpectrum("Transmission", sampleWsName + "_Trans", 0,
                               Qt::blue);
  m_uiForm.ppPlot->resizeX();

  // Enable plot and save
  m_uiForm.pbPlot->setEnabled(true);
  m_uiForm.pbSave->setEnabled(true);
}

void IndirectTransmission::instrumentSet() {
  auto const instrument = getInstrumentDetail("instrument");
  if (!instrument.isEmpty()) {
    m_uiForm.dsSampleInput->setInstrumentOverride(instrument);
    m_uiForm.dsCanInput->setInstrumentOverride(instrument);
  }
}

/**
 * Handle when Run is clicked
 */
void IndirectTransmission::runClicked() { runTab(); }

/**
 * Handle saving of workspace
 */
void IndirectTransmission::saveClicked() {
  QString outputWs =
      (m_uiForm.dsSampleInput->getCurrentDataName() + "_transmission");

  if (checkADSForPlotSaveWorkspace(outputWs.toStdString(), false))
    addSaveWorkspaceToQueue(outputWs);
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handle mantid plotting
 */
void IndirectTransmission::plotClicked() {
  setPlotIsPlotting(true);
  QString outputWs =
      (m_uiForm.dsSampleInput->getCurrentDataName() + "_transmission");
  if (checkADSForPlotSaveWorkspace(outputWs.toStdString(), true))
    plotSpectrum(outputWs);
  setPlotIsPlotting(false);
}

void IndirectTransmission::setRunEnabled(bool enabled) {
  m_uiForm.pbRun->setEnabled(enabled);
}

void IndirectTransmission::setPlotEnabled(bool enabled) {
  m_uiForm.pbPlot->setEnabled(enabled);
}

void IndirectTransmission::setSaveEnabled(bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
}

void IndirectTransmission::setOutputButtonsEnabled(
    std::string const &enableOutputButtons) {
  bool enable = enableOutputButtons == "enable" ? true : false;
  setPlotEnabled(enable);
  setSaveEnabled(enable);
}

void IndirectTransmission::updateRunButton(
    bool enabled, std::string const &enableOutputButtons, QString const message,
    QString const tooltip) {
  setRunEnabled(enabled);
  m_uiForm.pbRun->setText(message);
  m_uiForm.pbRun->setToolTip(tooltip);
  if (enableOutputButtons != "unchanged")
    setOutputButtonsEnabled(enableOutputButtons);
}

void IndirectTransmission::setPlotIsPlotting(bool plotting) {
  m_uiForm.pbPlot->setText(plotting ? "Plotting..." : "Plot Result");
  setPlotEnabled(!plotting);
  setRunEnabled(!plotting);
  setSaveEnabled(!plotting);
}

} // namespace CustomInterfaces
} // namespace MantidQt
