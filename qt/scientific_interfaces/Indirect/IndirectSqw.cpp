// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSqw.h"
#include "IndirectDataValidationHelper.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Plotting/AxisID.h"

#include <QFileInfo>

using namespace IndirectDataValidationHelper;
using namespace Mantid::API;
using MantidQt::API::BatchAlgorithmRunner;

namespace {
Mantid::Kernel::Logger g_log("S(Q,w)");

MatrixWorkspace_sptr getADSMatrixWorkspace(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      workspaceName);
}

double roundToPrecision(double value, double precision) {
  return value - std::remainder(value, precision);
}

std::pair<double, double>
roundToWidth(std::tuple<double, double> const &axisRange, double width) {
  return std::make_pair(roundToPrecision(std::get<0>(axisRange), width) + width,
                        roundToPrecision(std::get<1>(axisRange), width) -
                            width);
}

void convertToSpectrumAxis(std::string const &inputName,
                           std::string const &outputName) {
  auto converter = AlgorithmManager::Instance().create("ConvertSpectrumAxis");
  converter->initialize();
  converter->setProperty("InputWorkspace", inputName);
  converter->setProperty("OutputWorkspace", outputName);
  converter->setProperty("Target", "ElasticQ");
  converter->setProperty("EMode", "Indirect");
  converter->execute();
}

std::pair<double, double>
convertTupleToPair(std::tuple<double, double> const &tuple) {
  return std::make_pair(std::get<0>(tuple), std::get<1>(tuple));
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
//----------------------------------------------------------------------------------------------
/** Constructor
 */
IndirectSqw::IndirectSqw(IndirectDataReduction *idrUI, QWidget *parent)
    : IndirectDataReductionTab(idrUI, parent) {
  m_uiForm.setupUi(parent);
  m_plotOptionsPresenter = std::make_unique<IndirectPlotOptionsPresenter>(
      std::move(m_uiForm.ipoPlotOptions), this, PlotWidget::SpectraContour);

  connect(m_uiForm.dsSampleInput, SIGNAL(dataReady(QString const &)), this,
          SLOT(handleDataReady(QString const &)));
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(sqwAlgDone(bool)));

  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));

  connect(this,
          SIGNAL(updateRunButton(bool, std::string const &, QString const &,
                                 QString const &)),
          this,
          SLOT(updateRunButton(bool, std::string const &, QString const &,
                               QString const &)));

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  m_uiForm.rqwPlot2D->setXAxisLabel("Energy (meV)");
  m_uiForm.rqwPlot2D->setYAxisLabel("Q (A-1)");
#else
  m_uiForm.rqwPlot2D->setCanvasColour(QColor(240, 240, 240));
#endif

  // Allows empty workspace selector when initially selected
  m_uiForm.dsSampleInput->isOptional(true);

  // Disables searching for run files in the data archive
  m_uiForm.dsSampleInput->isForRunFiles(false);
}

IndirectSqw::~IndirectSqw() {}

void IndirectSqw::setup() {}

/**
 * Handles the event of data being loaded. Validates the loaded data.
 *
 */
void IndirectSqw::handleDataReady(QString const &dataName) {
  UserInputValidator uiv;
  validateDataIsOfType(uiv, m_uiForm.dsSampleInput, "Sample", DataType::Red);

  auto const errorMessage = uiv.generateErrorMessage();
  if (!errorMessage.isEmpty()) {
    showMessageBox(errorMessage);
  } else {
    plotRqwContour(dataName.toStdString());
    setDefaultQAndEnergy();
  }
}

bool IndirectSqw::validate() {
  double const tolerance = 1e-10;
  double const qLow = m_uiForm.spQLow->value();
  double const qWidth = m_uiForm.spQWidth->value();
  double const qHigh = m_uiForm.spQHigh->value();
  auto const qRange =
      m_uiForm.rqwPlot2D->getAxisRange(MantidWidgets::AxisID::YLeft);

  UserInputValidator uiv;

  // Validate the sample
  validateDataIsOfType(uiv, m_uiForm.dsSampleInput, "Sample", DataType::Red);

  // Validate Q binning
  uiv.checkBins(qLow, qWidth, qHigh, tolerance);
  uiv.checkRangeIsEnclosed("The contour plots Q axis",
                           convertTupleToPair(qRange), "the Q range provided",
                           std::make_pair(qLow, qHigh));

  // If selected, validate energy binning
  if (m_uiForm.ckRebinInEnergy->isChecked()) {
    double const eLow = m_uiForm.spELow->value();
    double const eWidth = m_uiForm.spEWidth->value();
    double const eHigh = m_uiForm.spEHigh->value();
    auto const eRange =
        m_uiForm.rqwPlot2D->getAxisRange(MantidWidgets::AxisID::XBottom);

    uiv.checkBins(eLow, eWidth, eHigh, tolerance);
    uiv.checkRangeIsEnclosed("The contour plots Energy axis",
                             convertTupleToPair(eRange), "the E range provided",
                             std::make_pair(eLow, eHigh));
  }

  auto const errorMessage = uiv.generateErrorMessage();

  // Show an error message if needed
  if (!errorMessage.isEmpty())
    emit showMessageBox(errorMessage);

  return errorMessage.isEmpty();
}

void IndirectSqw::run() {
  m_plotOptionsPresenter->removeWorkspace();

  auto const sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
  auto const sqwWsName = sampleWsName.left(sampleWsName.length() - 4) + "_sqw";
  auto const eRebinWsName = sampleWsName.left(sampleWsName.length() - 4) + "_r";

  auto const rebinString = m_uiForm.spQLow->text() + "," +
                           m_uiForm.spQWidth->text() + "," +
                           m_uiForm.spQHigh->text();

  // Rebin in energy
  bool const rebinInEnergy = m_uiForm.ckRebinInEnergy->isChecked();
  if (rebinInEnergy) {
    auto const eRebinString = m_uiForm.spELow->text() + "," +
                              m_uiForm.spEWidth->text() + "," +
                              m_uiForm.spEHigh->text();

    auto energyRebinAlg = AlgorithmManager::Instance().create("Rebin");
    energyRebinAlg->initialize();
    energyRebinAlg->setProperty("InputWorkspace", sampleWsName.toStdString());
    energyRebinAlg->setProperty("OutputWorkspace", eRebinWsName.toStdString());
    energyRebinAlg->setProperty("Params", eRebinString.toStdString());

    m_batchAlgoRunner->addAlgorithm(energyRebinAlg);
  }

  auto const eFixed = getInstrumentDetail("Efixed").toStdString();

  auto sqwAlg = AlgorithmManager::Instance().create("SofQW");
  sqwAlg->initialize();
  sqwAlg->setProperty("OutputWorkspace", sqwWsName.toStdString());
  sqwAlg->setProperty("QAxisBinning", rebinString.toStdString());
  sqwAlg->setProperty("EMode", "Indirect");
  sqwAlg->setProperty("EFixed", eFixed);
  sqwAlg->setProperty("Method", "NormalisedPolygon");
  sqwAlg->setProperty("ReplaceNaNs", true);

  BatchAlgorithmRunner::AlgorithmRuntimeProps sqwInputProps;
  sqwInputProps["InputWorkspace"] =
      rebinInEnergy ? eRebinWsName.toStdString() : sampleWsName.toStdString();

  m_batchAlgoRunner->addAlgorithm(sqwAlg, sqwInputProps);

  // Add sample log for S(Q, w) algorithm used
  auto sampleLogAlg = AlgorithmManager::Instance().create("AddSampleLog");
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

std::size_t IndirectSqw::getOutWsNumberOfSpectra() const {
  return getADSMatrixWorkspace(m_pythonExportWsName)->getNumberHistograms();
}

/**
 * Handles plotting the S(Q, w) workspace when the algorithm chain is finished.
 *
 * @param error If the algorithm chain failed
 */
void IndirectSqw::sqwAlgDone(bool error) {
  if (!error) {
    m_plotOptionsPresenter->setWorkspaces({m_pythonExportWsName});
    setSaveEnabled(true);
  }
}

/**
 * Plots the data as a contour plot
 *
 * Creates a colour 2D plot of the data
 */
void IndirectSqw::plotRqwContour(std::string const &sampleName) {
  auto const outputName = sampleName.substr(0, sampleName.size() - 4) + "_rqw";

  try {
    convertToSpectrumAxis(sampleName, outputName);
    if (AnalysisDataService::Instance().doesExist(outputName)) {
      auto const rqwWorkspace = getADSMatrixWorkspace(outputName);
      if (rqwWorkspace)
        m_uiForm.rqwPlot2D->setWorkspace(rqwWorkspace);
    }
  } catch (std::exception const &ex) {
    g_log.warning(ex.what());
    showMessageBox("Invalid file. Please load a valid reduced workspace.");
  }
}

void IndirectSqw::setDefaultQAndEnergy() {
  setQRange(m_uiForm.rqwPlot2D->getAxisRange(MantidWidgets::AxisID::YLeft));
  setEnergyRange(
      m_uiForm.rqwPlot2D->getAxisRange(MantidWidgets::AxisID::XBottom));
}

void IndirectSqw::setQRange(std::tuple<double, double> const &axisRange) {
  auto const qRange = roundToWidth(axisRange, m_uiForm.spQWidth->value());
  m_uiForm.spQLow->setValue(qRange.first);
  m_uiForm.spQHigh->setValue(qRange.second);
}

void IndirectSqw::setEnergyRange(std::tuple<double, double> const &axisRange) {
  auto const energyRange = roundToWidth(axisRange, m_uiForm.spEWidth->value());
  m_uiForm.spELow->setValue(energyRange.first);
  m_uiForm.spEHigh->setValue(energyRange.second);
}

void IndirectSqw::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("Sqw");
  m_uiForm.dsSampleInput->setFBSuffixes(filter ? getSampleFBSuffixes(tabName)
                                               : getExtensions(tabName));
  m_uiForm.dsSampleInput->setWSSuffixes(filter ? getSampleWSSuffixes(tabName)
                                               : noSuffixes);
}

void IndirectSqw::runClicked() { runTab(); }

void IndirectSqw::saveClicked() {
  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName));
  m_batchAlgoRunner->executeBatch();
}

void IndirectSqw::setRunEnabled(bool enabled) {
  m_uiForm.pbRun->setEnabled(enabled);
}

void IndirectSqw::setSaveEnabled(bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
}

void IndirectSqw::setOutputButtonsEnabled(
    std::string const &enableOutputButtons) {
  setSaveEnabled(enableOutputButtons == "enable");
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

} // namespace CustomInterfaces
} // namespace MantidQt
