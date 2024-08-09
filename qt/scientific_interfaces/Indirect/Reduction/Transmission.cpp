// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Transmission.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <QFileInfo>

using namespace Mantid::API;

namespace {

void conjoinSpectra(std::string const &inputWorkspaces, std::string const &outputName) {
  auto conjoin = AlgorithmManager::Instance().create("ConjoinSpectra");
  conjoin->initialize();
  conjoin->setProperty("InputWorkspaces", inputWorkspaces);
  conjoin->setProperty("OutputWorkspace", outputName);
  conjoin->execute();
}

} // namespace

namespace MantidQt::CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
Transmission::Transmission(IDataReduction *idrUI, QWidget *parent) : DataReductionTab(idrUI, parent) {
  m_uiForm.setupUi(parent);
  setRunWidgetPresenter(std::make_unique<RunPresenter>(this, m_uiForm.runWidget));
  setOutputPlotOptionsPresenter(
      std::make_unique<OutputPlotOptionsPresenter>(m_uiForm.ipoPlotOptions, PlotWidget::Spectra, "0-2"));

  // Update the preview plot when the algorithm is complete
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(transAlgDone(bool)));

  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));

  m_uiForm.ppPlot->setCanvasColour(QColor(240, 240, 240));

  m_uiForm.dsSampleInput->setTypeSelectorVisible(false);
  m_uiForm.dsCanInput->setTypeSelectorVisible(false);
}

Transmission::~Transmission() = default;

void Transmission::handleRun() {
  QString sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
  QString canWsName = m_uiForm.dsCanInput->getCurrentDataName();
  QString outWsName = sampleWsName.toLower() + "_transmission_group";

  IAlgorithm_sptr transAlg = AlgorithmManager::Instance().create("IndirectTransmissionMonitor", -1);
  transAlg->initialize();

  transAlg->setProperty("SampleWorkspace", sampleWsName.toStdString());
  transAlg->setProperty("CanWorkspace", canWsName.toStdString());
  transAlg->setProperty("OutputWorkspace", outWsName.toStdString());

  m_batchAlgoRunner->addAlgorithm(transAlg);
  m_batchAlgoRunner->executeBatchAsync();

  m_pythonExportWsName = outWsName.toStdString();
}

void Transmission::handleValidation(IUserInputValidator *validator) const {
  // Check if we have an appropriate instrument
  QString currentInst = getInstrumentName();
  if (currentInst != "IRIS" && currentInst != "OSIRIS")
    validator->addErrorMessage("The selected instrument must be IRIS or OSIRIS");

  // Check for an invalid sample input
  if (!m_uiForm.dsSampleInput->isValid())
    validator->addErrorMessage("Sample: " + m_uiForm.dsSampleInput->getProblem().toStdString());

  // Check for an invalid can input
  if (!m_uiForm.dsCanInput->isValid())
    validator->addErrorMessage("Resolution: " + m_uiForm.dsCanInput->getProblem().toStdString());
}

void Transmission::transAlgDone(bool error) {
  m_runPresenter->setRunEnabled(true);
  m_uiForm.pbSave->setEnabled(!error);
  if (error)
    return;

  auto const sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
  auto const transmissionName = sampleWsName.toLower().toStdString() + "_transmission";
  conjoinSpectra(sampleWsName.toStdString() + "_Can," + sampleWsName.toStdString() + "_Sam," +
                     sampleWsName.toStdString() + "_Trans",
                 transmissionName);

  setOutputPlotOptionsWorkspaces({transmissionName});

  // Do plotting
  m_uiForm.ppPlot->clear();
  m_uiForm.ppPlot->addSpectrum("Can", sampleWsName + "_Can", 0, Qt::black);
  m_uiForm.ppPlot->addSpectrum("Sample", sampleWsName + "_Sam", 0, Qt::red);
  m_uiForm.ppPlot->addSpectrum("Transmission", sampleWsName + "_Trans", 0, Qt::blue);
  m_uiForm.ppPlot->resizeX();
}

void Transmission::updateInstrumentConfiguration() {
  try {
    setInstrument(getInstrumentDetail("instrument"));
  } catch (std::exception const &ex) {
    showMessageBox(ex.what());
  }
}

void Transmission::setInstrument(QString const &instrumentName) {
  m_uiForm.dsSampleInput->setInstrumentOverride(instrumentName);
  m_uiForm.dsCanInput->setInstrumentOverride(instrumentName);
}

/**
 * Handle saving of workspace
 */
void Transmission::saveClicked() {
  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(m_pythonExportWsName);
  m_batchAlgoRunner->executeBatchAsync();
}

void Transmission::setSaveEnabled(bool enabled) { m_uiForm.pbSave->setEnabled(enabled); }

void Transmission::setLoadHistory(bool doLoadHistory) {
  m_uiForm.dsSampleInput->setLoadProperty("LoadHistory", doLoadHistory);
  m_uiForm.dsCanInput->setLoadProperty("LoadHistory", doLoadHistory);
};

} // namespace MantidQt::CustomInterfaces
