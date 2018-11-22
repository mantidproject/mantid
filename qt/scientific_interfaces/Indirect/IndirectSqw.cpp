// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSqw.h"
#include "../General/UserInputValidator.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <QFileInfo>

using namespace Mantid::API;
using MantidQt::API::BatchAlgorithmRunner;

namespace MantidQt {
namespace CustomInterfaces {
//----------------------------------------------------------------------------------------------
/** Constructor
 */
IndirectSqw::IndirectSqw(IndirectDataReduction *idrUI, QWidget *parent)
    : IndirectDataReductionTab(idrUI, parent) {
  m_uiForm.setupUi(parent);

  connect(m_uiForm.dsSampleInput, SIGNAL(loadClicked()), this,
          SLOT(plotContour()));
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(sqwAlgDone(bool)));

  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbPlotSpectrum, SIGNAL(clicked()), this,
          SLOT(plotSpectrumClicked()));
  connect(m_uiForm.pbPlotContour, SIGNAL(clicked()), this,
          SLOT(plotContourClicked()));
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
IndirectSqw::~IndirectSqw() {}

void IndirectSqw::setup() {}

bool IndirectSqw::validate() {
  double tolerance = 1e-10;
  UserInputValidator uiv;

  // Validate the data selector
  uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSampleInput);

  // Validate Q binning
  uiv.checkBins(m_uiForm.spQLow->value(), m_uiForm.spQWidth->value(),
                m_uiForm.spQHigh->value(), tolerance);

  // If selected, validate energy binning
  if (m_uiForm.ckRebinInEnergy->isChecked())
    uiv.checkBins(m_uiForm.spELow->value(), m_uiForm.spEWidth->value(),
                  m_uiForm.spEHigh->value(), tolerance);

  QString errorMessage = uiv.generateErrorMessage();

  // Show an error message if needed
  if (!errorMessage.isEmpty())
    emit showMessageBox(errorMessage);

  return errorMessage.isEmpty();
}

void IndirectSqw::run() {
  QString sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
  QString sqwWsName = sampleWsName.left(sampleWsName.length() - 4) + "_sqw";
  QString eRebinWsName = sampleWsName.left(sampleWsName.length() - 4) + "_r";

  QString rebinString = m_uiForm.spQLow->text() + "," +
                        m_uiForm.spQWidth->text() + "," +
                        m_uiForm.spQHigh->text();

  // Rebin in energy
  bool rebinInEnergy = m_uiForm.ckRebinInEnergy->isChecked();
  if (rebinInEnergy) {
    QString eRebinString = m_uiForm.spELow->text() + "," +
                           m_uiForm.spEWidth->text() + "," +
                           m_uiForm.spEHigh->text();

    IAlgorithm_sptr energyRebinAlg =
        AlgorithmManager::Instance().create("Rebin");
    energyRebinAlg->initialize();

    energyRebinAlg->setProperty("InputWorkspace", sampleWsName.toStdString());
    energyRebinAlg->setProperty("OutputWorkspace", eRebinWsName.toStdString());
    energyRebinAlg->setProperty("Params", eRebinString.toStdString());

    m_batchAlgoRunner->addAlgorithm(energyRebinAlg);
  }

  QString eFixed = getInstrumentDetails()["Efixed"];

  IAlgorithm_sptr sqwAlg = AlgorithmManager::Instance().create("SofQW");
  sqwAlg->initialize();

  BatchAlgorithmRunner::AlgorithmRuntimeProps sqwInputProps;
  if (rebinInEnergy)
    sqwInputProps["InputWorkspace"] = eRebinWsName.toStdString();
  else
    sqwInputProps["InputWorkspace"] = sampleWsName.toStdString();

  sqwAlg->setProperty("OutputWorkspace", sqwWsName.toStdString());
  sqwAlg->setProperty("QAxisBinning", rebinString.toStdString());
  sqwAlg->setProperty("EMode", "Indirect");
  sqwAlg->setProperty("EFixed", eFixed.toStdString());
  sqwAlg->setProperty("Method", "NormalisedPolygon");
  sqwAlg->setProperty("ReplaceNaNs", true);

  m_batchAlgoRunner->addAlgorithm(sqwAlg, sqwInputProps);

  // Add sample log for S(Q, w) algorithm used
  IAlgorithm_sptr sampleLogAlg =
      AlgorithmManager::Instance().create("AddSampleLog");
  sampleLogAlg->initialize();

  sampleLogAlg->setProperty("LogName", "rebin_type");
  sampleLogAlg->setProperty("LogType", "String");
  sampleLogAlg->setProperty("LogText", "NormalisedPolygon");

  BatchAlgorithmRunner::AlgorithmRuntimeProps inputToAddSampleLogProps;
  inputToAddSampleLogProps["Workspace"] = sqwWsName.toStdString();

  m_batchAlgoRunner->addAlgorithm(sampleLogAlg, inputToAddSampleLogProps);

  // Set the name of the result workspace for Python export
  m_pythonExportWsName = sqwWsName.toStdString();

  m_batchAlgoRunner->executeBatch();
}

MatrixWorkspace_const_sptr
IndirectSqw::getADSWorkspace(std::string const &name) const {
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name);
}

std::size_t IndirectSqw::getOutWsNumberOfSpectra() const {
  return getADSWorkspace(m_pythonExportWsName)->getNumberHistograms();
}

/**
 * Handles plotting the S(Q, w) workspace when the algorithm chain is finished.
 *
 * @param error If the algorithm chain failed
 */
void IndirectSqw::sqwAlgDone(bool error) {
  if (!error) {
    setPlotSpectrumEnabled(true);
    setPlotContourEnabled(true);
    setSaveEnabled(true);

    setPlotSpectrumIndexMax(static_cast<int>(getOutWsNumberOfSpectra()) - 1);
  }
}

void IndirectSqw::setPlotSpectrumIndexMax(int maximum) {
  MantidQt::API::SignalBlocker<QObject> blocker(m_uiForm.spSpectrum);
  m_uiForm.spSpectrum->setMaximum(maximum);
}

/**
 * Handles the Plot Input button
 *
 * Creates a colour 2D plot of the data
 */
void IndirectSqw::plotContour() {
  if (m_uiForm.dsSampleInput->isValid()) {
    QString sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();

    QString convertedWsName =
        sampleWsName.left(sampleWsName.length() - 4) + "_rqw";

    IAlgorithm_sptr convertSpecAlg =
        AlgorithmManager::Instance().create("ConvertSpectrumAxis");
    convertSpecAlg->initialize();

    convertSpecAlg->setProperty("InputWorkspace", sampleWsName.toStdString());
    convertSpecAlg->setProperty("OutputWorkspace",
                                convertedWsName.toStdString());
    convertSpecAlg->setProperty("Target", "ElasticQ");
    convertSpecAlg->setProperty("EMode", "Indirect");

    convertSpecAlg->execute();

    QString pyInput = "plot2D('" + convertedWsName + "')\n";
    m_pythonRunner.runPythonCode(pyInput);
  } else {
    emit showMessageBox("Invalid filename.");
  }
}

void IndirectSqw::runClicked() { runTab(); }

void IndirectSqw::plotSpectrumClicked() {
  setPlotSpectrumIsPlotting(true);

  auto const spectrumNumber = m_uiForm.spSpectrum->text().toInt();
  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, true))
    plotSpectrum(QString::fromStdString(m_pythonExportWsName), spectrumNumber);

  setPlotSpectrumIsPlotting(false);
}

void IndirectSqw::plotContourClicked() {
  setPlotContourIsPlotting(true);

	if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, true)) {
		QString pyInput = "from mantidplot import plot2D\nimportMatrixWorkspace('" + QString::fromStdString(m_pythonExportWsName) + "').plotGraph2D()\n";
		m_pythonRunner.runPythonCode(pyInput);
	}

  setPlotContourIsPlotting(false);
}

void IndirectSqw::saveClicked() {
  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName));
  m_batchAlgoRunner->executeBatch();
}

void IndirectSqw::setRunEnabled(bool enabled) {
  m_uiForm.pbRun->setEnabled(enabled);
}

void IndirectSqw::setPlotSpectrumEnabled(bool enabled) {
  m_uiForm.pbPlotSpectrum->setEnabled(enabled);
  m_uiForm.spSpectrum->setEnabled(enabled);
}

void IndirectSqw::setPlotContourEnabled(bool enabled) {
  m_uiForm.pbPlotContour->setEnabled(enabled);
}

void IndirectSqw::setSaveEnabled(bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
}

void IndirectSqw::setOutputButtonsEnabled(
    std::string const &enableOutputButtons) {
  bool enable = enableOutputButtons == "enable" ? true : false;
  setPlotSpectrumEnabled(enable);
  setPlotContourEnabled(enable);
  setSaveEnabled(enable);
}

void IndirectSqw::updateRunButton(bool enabled,
                                  std::string const &enableOutputButtons,
                                  QString const message,
                                  QString const tooltip) {
  setRunEnabled(enabled);
  m_uiForm.pbRun->setText(message);
  m_uiForm.pbRun->setToolTip(tooltip);
  if (enableOutputButtons != "unchanged")
    setOutputButtonsEnabled(enableOutputButtons);
}

void IndirectSqw::setPlotSpectrumIsPlotting(bool plotting) {
  m_uiForm.pbPlotSpectrum->setText(plotting ? "Plotting..." : "Plot Spectrum");
  setPlotSpectrumEnabled(!plotting);
  setPlotContourEnabled(!plotting);
  setRunEnabled(!plotting);
  setSaveEnabled(!plotting);
}

void IndirectSqw::setPlotContourIsPlotting(bool plotting) {
  m_uiForm.pbPlotContour->setText(plotting ? "Plotting..." : "Plot Contour");
  setPlotSpectrumEnabled(!plotting);
  setPlotContourEnabled(!plotting);
  setRunEnabled(!plotting);
  setSaveEnabled(!plotting);
}

} // namespace CustomInterfaces
} // namespace MantidQt
