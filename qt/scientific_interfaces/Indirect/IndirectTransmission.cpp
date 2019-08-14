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

namespace {

void conjoinSpectra(std::string const &inputWorkspaces,
                    std::string const &outputName) {
  auto conjoin = AlgorithmManager::Instance().create("ConjoinSpectra");
  conjoin->initialize();
  conjoin->setProperty("InputWorkspaces", inputWorkspaces);
  conjoin->setProperty("OutputWorkspace", outputName);
  conjoin->execute();
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
IndirectTransmission::IndirectTransmission(IndirectDataReduction *idrUI,
                                           QWidget *parent)
    : IndirectDataReductionTab(idrUI, parent) {
  m_uiForm.setupUi(parent);
  setOutputPlotOptionsPresenter(std::make_unique<IndirectPlotOptionsPresenter>(
      m_uiForm.ipoPlotOptions, this, PlotWidget::Spectra, "0-2"));

  connect(this, SIGNAL(newInstrumentConfiguration()), this,
          SLOT(setInstrument()));

  // Update the preview plot when the algorithm is complete
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(transAlgDone(bool)));

  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));

  connect(this,
          SIGNAL(updateRunButton(bool, std::string const &, QString const &,
                                 QString const &)),
          this,
          SLOT(updateRunButton(bool, std::string const &, QString const &,
                               QString const &)));

  m_uiForm.ppPlot->setCanvasColour(QColor(240, 240, 240));

  m_uiForm.dsSampleInput->setTypeSelectorVisible(false);
  m_uiForm.dsCanInput->setTypeSelectorVisible(false);
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
IndirectTransmission::~IndirectTransmission() {}

void IndirectTransmission::setup() {}

void IndirectTransmission::run() {
  QString sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
  QString canWsName = m_uiForm.dsCanInput->getCurrentDataName();
  QString outWsName = sampleWsName.toLower() + "_transmission_group";

  IAlgorithm_sptr transAlg =
      AlgorithmManager::Instance().create("IndirectTransmissionMonitor", -1);
  transAlg->initialize();

  transAlg->setProperty("SampleWorkspace", sampleWsName.toStdString());
  transAlg->setProperty("CanWorkspace", canWsName.toStdString());
  transAlg->setProperty("OutputWorkspace", outWsName.toStdString());

  m_batchAlgoRunner->addAlgorithm(transAlg);
  m_batchAlgoRunner->executeBatchAsync();

  m_pythonExportWsName = outWsName.toStdString();
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

  auto const sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
  auto const transmissionName =
      sampleWsName.toLower().toStdString() + "_transmission";
  conjoinSpectra(sampleWsName.toStdString() + "_Can," +
                     sampleWsName.toStdString() + "_Sam," +
                     sampleWsName.toStdString() + "_Trans",
                 transmissionName);

  setOutputPlotOptionsWorkspaces({transmissionName});

  // Do plotting
  m_uiForm.ppPlot->clear();
  m_uiForm.ppPlot->addSpectrum("Can", sampleWsName + "_Can", 0, Qt::black);
  m_uiForm.ppPlot->addSpectrum("Sample", sampleWsName + "_Sam", 0, Qt::red);
  m_uiForm.ppPlot->addSpectrum("Transmission", sampleWsName + "_Trans", 0,
                               Qt::blue);
  m_uiForm.ppPlot->resizeX();

  // Enable plot and save
  m_uiForm.pbSave->setEnabled(true);
}

void IndirectTransmission::setInstrument() {
  try {
    setInstrument(getInstrumentDetail("instrument"));
  } catch (std::exception const &ex) {
    showMessageBox(ex.what());
  }
}

void IndirectTransmission::setInstrument(QString const &instrumentName) {
  m_uiForm.dsSampleInput->setInstrumentOverride(instrumentName);
  m_uiForm.dsCanInput->setInstrumentOverride(instrumentName);
}

/**
 * Handle when Run is clicked
 */
void IndirectTransmission::runClicked() { runTab(); }

/**
 * Handle saving of workspace
 */
void IndirectTransmission::saveClicked() {
  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(m_pythonExportWsName);
  m_batchAlgoRunner->executeBatchAsync();
}

void IndirectTransmission::setRunEnabled(bool enabled) {
  m_uiForm.pbRun->setEnabled(enabled);
}

void IndirectTransmission::setSaveEnabled(bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
}

void IndirectTransmission::updateRunButton(
    bool enabled, std::string const &enableOutputButtons, QString const message,
    QString const tooltip) {
  setRunEnabled(enabled);
  m_uiForm.pbRun->setText(message);
  m_uiForm.pbRun->setToolTip(tooltip);
  if (enableOutputButtons != "unchanged")
    setSaveEnabled(enableOutputButtons == "enable");
}

} // namespace CustomInterfaces
} // namespace MantidQt
