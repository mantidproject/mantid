// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ContainerSubtraction.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/Unit.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("ContainerSubtraction");
}

namespace MantidQt {
namespace CustomInterfaces {
ContainerSubtraction::ContainerSubtraction(QWidget *parent)
    : CorrectionsTab(parent), m_spectra(0) {
  m_uiForm.setupUi(parent);
  setOutputPlotOptionsPresenter(std::make_unique<IndirectPlotOptionsPresenter>(
      m_uiForm.ipoPlotOptions, this, PlotWidget::SpectraContour));

  connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString &)), this,
          SLOT(newSample(const QString &)));
  connect(m_uiForm.dsContainer, SIGNAL(dataReady(const QString &)), this,
          SLOT(newContainer(const QString &)));
  connect(m_uiForm.spPreviewSpec, SIGNAL(valueChanged(int)), this,
          SLOT(plotPreview(int)));
  connect(m_uiForm.spCanScale, SIGNAL(valueChanged(double)), this,
          SLOT(updateCan()));
  connect(m_uiForm.spShift, SIGNAL(valueChanged(double)), this,
          SLOT(updateCan()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbPlotPreview, SIGNAL(clicked()), this,
          SLOT(plotCurrentPreview()));

  // Allows empty workspace selector when initially selected
  m_uiForm.dsSample->isOptional(true);
  m_uiForm.dsContainer->isOptional(true);

  m_uiForm.spPreviewSpec->setMinimum(0);
  m_uiForm.spPreviewSpec->setMaximum(0);
}

ContainerSubtraction::~ContainerSubtraction() {
  m_uiForm.ppPreview->watchADS(false);
  if (m_transformedContainerWS)
    AnalysisDataService::Instance().remove(m_transformedContainerWS->getName());
}

void ContainerSubtraction::setTransformedContainer(
    MatrixWorkspace_sptr workspace, const std::string &name) {
  m_transformedContainerWS = workspace;
  AnalysisDataService::Instance().addOrReplace(name, m_transformedContainerWS);
}

void ContainerSubtraction::setTransformedContainer(
    MatrixWorkspace_sptr workspace) {
  m_transformedContainerWS = workspace;
  AnalysisDataService::Instance().addOrReplace(workspace->getName(),
                                               m_transformedContainerWS);
}

void ContainerSubtraction::setup() {}

void ContainerSubtraction::run() {
  setRunIsRunning(true);

  if (m_csSampleWS && m_csContainerWS) {
    m_originalSampleUnits = m_csSampleWS->getAxis(0)->unit()->unitID();

    // Check if using shift / scale
    const bool shift = m_uiForm.ckShiftCan->isChecked();
    const bool scale = m_uiForm.ckScaleCan->isChecked();

    auto containerWs = m_csContainerWS;
    if (shift) {
      containerWs = shiftWorkspace(containerWs, m_uiForm.spShift->value());
      containerWs = rebinToWorkspace(containerWs, m_csSampleWS);
    } else if (!checkWorkspaceBinningMatches(m_csSampleWS, containerWs)) {
      containerWs = requestRebinToSample(containerWs);

      if (!checkWorkspaceBinningMatches(m_csSampleWS, containerWs)) {
        setRunIsRunning(false);
        setSaveResultEnabled(false);
        g_log.error("Cannot apply container corrections using a sample and "
                    "container with different binning.");
        return;
      }
    }

    if (scale)
      containerWs = scaleWorkspace(containerWs, m_uiForm.spCanScale->value());

    m_csSubtractedWS = minusWorkspace(m_csSampleWS, containerWs);
    m_pythonExportWsName = createOutputName();
    AnalysisDataService::Instance().addOrReplace(m_pythonExportWsName,
                                                 m_csSubtractedWS);
    containerSubtractionComplete();
  }
  setRunIsRunning(false);
  setOutputPlotOptionsWorkspaces({m_pythonExportWsName});
}

std::string ContainerSubtraction::createOutputName() {
  QString QStrContainerWs = QString::fromStdString(m_csContainerWS->getName());
  auto QStrSampleWs = QString::fromStdString(m_csSampleWS->getName());
  int sampleNameCutIndex = QStrSampleWs.lastIndexOf("_");
  if (sampleNameCutIndex == -1)
    sampleNameCutIndex = QStrSampleWs.length();

  std::string runNum = "";
  int containerNameCutIndex = 0;
  if (m_csContainerWS->run().hasProperty("run_number")) {
    runNum = m_csContainerWS->run().getProperty("run_number")->value();
  } else {
    containerNameCutIndex = QStrContainerWs.indexOf("_");
    if (containerNameCutIndex == -1)
      containerNameCutIndex = QStrContainerWs.length();
  }

  QString outputWsName = QStrSampleWs.left(sampleNameCutIndex) + "_Subtract_";
  if (runNum.compare("") != 0) {
    outputWsName += QString::fromStdString(runNum);
  } else {
    outputWsName += QStrContainerWs.left(containerNameCutIndex);
  }

  outputWsName += "_red";
  return outputWsName.toStdString();
}

/**
 * Removes the output workspace from the ADS if exists
 */
void ContainerSubtraction::removeOutput() {
  auto &ads = AnalysisDataService::Instance();
  if (ads.doesExist(m_pythonExportWsName)) {
    ads.remove(m_pythonExportWsName);
  }
  m_pythonExportWsName.clear();
}

/**
 * Validates the user input in the UI
 * @return if the input was valid
 */
bool ContainerSubtraction::validate() {
  UserInputValidator uiv;

  // Check valid inputs
  uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSample);
  uiv.checkDataSelectorIsValid("Container", m_uiForm.dsContainer);

  // Check sample is a matrix workspace
  const auto sampleName = m_uiForm.dsSample->getCurrentDataName();
  const auto sampleWsName = sampleName.toStdString();
  bool sampleExists = AnalysisDataService::Instance().doesExist(sampleWsName);
  if (sampleExists &&
      !AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          sampleWsName)) {
    uiv.addErrorMessage(
        "Invalid sample workspace. Ensure a MatrixWorkspace is provided.");
  }

  // Check container is a matrix workspace
  const auto containerName = m_uiForm.dsContainer->getCurrentDataName();
  const auto containerWsName = containerName.toStdString();
  bool containerExists =
      AnalysisDataService::Instance().doesExist(containerWsName);
  if (containerExists &&
      !AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          containerWsName)) {
    uiv.addErrorMessage(
        "Invalid container workspace. Ensure a MatrixWorkspace is provided.");
  }

  if (m_csSampleWS && m_csContainerWS) {
    // Check Sample is of same type as container
    const auto containerType = m_csContainerWS->YUnit();
    const auto sampleType = m_csSampleWS->YUnit();

    g_log.debug() << "Sample Y-Unit is: " << sampleType << '\n';
    g_log.debug() << "Container Y-Unit is: " << containerType << '\n';

    if (containerType != sampleType)
      uiv.addErrorMessage("Sample and can workspaces must contain the same "
                          "type of data; have the same Y-Unit.");

    // Check sample has the same number of Histograms as the contianer
    const size_t sampleHist = m_csSampleWS->getNumberHistograms();
    const size_t containerHist = m_csContainerWS->getNumberHistograms();

    if (sampleHist != containerHist) {
      uiv.addErrorMessage(
          " Sample and Container do not have a matching number of Histograms.");
    }
  }

  // Show errors if there are any
  if (!uiv.isAllInputValid())
    emit showMessageBox(uiv.generateErrorMessage());

  return uiv.isAllInputValid();
}

void ContainerSubtraction::loadSettings(const QSettings &settings) {
  m_uiForm.dsContainer->readSettings(settings.group());
  m_uiForm.dsSample->readSettings(settings.group());
}

void ContainerSubtraction::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("ContainerSubtraction");
  m_uiForm.dsSample->setFBSuffixes(filter ? getSampleFBSuffixes(tabName)
                                          : getExtensions(tabName));
  m_uiForm.dsSample->setWSSuffixes(filter ? getSampleWSSuffixes(tabName)
                                          : noSuffixes);
  m_uiForm.dsContainer->setFBSuffixes(filter ? getContainerFBSuffixes(tabName)
                                             : getExtensions(tabName));
  m_uiForm.dsContainer->setWSSuffixes(filter ? getContainerWSSuffixes(tabName)
                                             : noSuffixes);
}

/**
 * Displays the sample data on the plot preview
 * @param dataName Name of new data source
 */
void ContainerSubtraction::newSample(const QString &dataName) {
  // Remove old sample and fit curves from plot
  m_uiForm.ppPreview->removeSpectrum("Subtracted");
  m_uiForm.ppPreview->removeSpectrum("Sample");
  // Remove the subtracted workspace as it is no longer valid
  removeOutput();

  m_csSampleWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      dataName.toStdString());
  // Get new workspace
  if (m_csSampleWS) {
    m_csSampleWS = convertToHistogram(m_csSampleWS);
    m_uiForm.spPreviewSpec->setMaximum(
        static_cast<int>(m_csSampleWS->getNumberHistograms()) - 1);

    // Plot the sample curve
    plotInPreview("Sample", m_csSampleWS, Qt::black);

    // Set min/max container shift
    auto min = m_csSampleWS->getXMin();
    auto max = m_csSampleWS->getXMax();

    m_uiForm.spShift->setMinimum(min);
    m_uiForm.spShift->setMaximum(max);
  } else {
    displayInvalidWorkspaceTypeError(dataName.toStdString(), g_log);
  }
}

/**
 * Displays the container data on the plot preview
 * @param dataName Name of new data source
 */
void ContainerSubtraction::newContainer(const QString &dataName) {
  // Remove old container and fit
  m_uiForm.ppPreview->removeSpectrum("Subtracted");
  m_uiForm.ppPreview->removeSpectrum("Container");
  // Remove the subtracted workspace as it is no longer valid
  removeOutput();

  // Get new workspace
  m_csContainerWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      dataName.toStdString());

  if (m_csContainerWS) {
    m_csContainerWS = convertToHistogram(m_csContainerWS);
    setTransformedContainer(m_csContainerWS);

    // Plot new container
    plotInPreview("Container", m_csContainerWS, Qt::red);
  } else {
    displayInvalidWorkspaceTypeError(dataName.toStdString(), g_log);
  }
}

/**
 * Handles Container curve in the miniplot when scale or shift is updated
 */
void ContainerSubtraction::updateCan() {
  auto shift = m_uiForm.ckShiftCan->isChecked();
  auto scale = m_uiForm.ckScaleCan->isChecked();
  auto transformed = m_csContainerWS;

  if (transformed) {

    if (shift) {
      transformed = shiftWorkspace(transformed, m_uiForm.spShift->value());
      transformed = rebinToWorkspace(transformed, m_csSampleWS);
    } else if (m_csSampleWS &&
               !checkWorkspaceBinningMatches(m_csSampleWS, m_csContainerWS)) {
      transformed = rebinToWorkspace(transformed, m_csSampleWS);
    }

    if (scale)
      transformed = scaleWorkspace(transformed, m_uiForm.spCanScale->value());
    setTransformedContainer(transformed,
                            "__" + m_csContainerWS->getName() + "_transformed");
  }
  plotPreview(m_uiForm.spPreviewSpec->value());
}

/**
 * Replots the preview plot.
 *
 * @param wsIndex workspace index to plot
 */
void ContainerSubtraction::plotPreview(int wsIndex) {
  m_uiForm.ppPreview->clear();
  m_uiForm.ppPreview->setUpdatesEnabled(false);

  // Plot container
  if (m_csContainerWS) {
    m_uiForm.ppPreview->addSpectrum("Container", m_transformedContainerWS,
                                    wsIndex, Qt::red);
  }

  // Plot sample
  if (m_csSampleWS) {
    m_uiForm.ppPreview->addSpectrum("Sample", m_csSampleWS, wsIndex, Qt::black);
  }

  // Plot result
  if (!m_pythonExportWsName.empty()) {
    m_uiForm.ppPreview->addSpectrum(
        "Subtracted", QString::fromStdString(m_pythonExportWsName), wsIndex,
        Qt::blue);
  }
  m_uiForm.ppPreview->setUpdatesEnabled(true);
  m_spectra = boost::numeric_cast<size_t>(wsIndex);
}

/**
 * Handles completion of the abs. correction algorithm.
 *
 * @param error True if algorithm failed.
 */
void ContainerSubtraction::containerSubtractionComplete() {
  plotPreview(m_uiForm.spPreviewSpec->value());

  if (m_uiForm.ckShiftCan->isChecked()) {
    auto logText = boost::lexical_cast<std::string>(m_uiForm.spShift->value());
    auto shiftLog = addSampleLogAlgorithm(m_csSubtractedWS, "container_shift",
                                          "Number", logText);
    m_batchAlgoRunner->addAlgorithm(shiftLog);
  }
}

void ContainerSubtraction::saveClicked() {

  // Check workspace exists
  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName));
  m_batchAlgoRunner->executeBatchAsync();
}

void ContainerSubtraction::runClicked() {
  clearOutputPlotOptionsWorkspaces();
  runTab();
}

/**
 * Plots the current spectrum displayed in the preview plot
 */
void ContainerSubtraction::plotCurrentPreview() {
  std::vector<std::string> workspaces;
  auto const index = boost::numeric_cast<int>(m_spectra);
  std::vector<int> indices;
  // Check whether a sample workspace has been specified
  if (m_csSampleWS) {
    workspaces.emplace_back(m_csSampleWS->getName());
    indices.emplace_back(index);
  }

  // Check whether a container workspace has been specified
  if (m_transformedContainerWS) {
    workspaces.emplace_back(m_transformedContainerWS->getName());
    indices.emplace_back(index);
  }

  // Check whether a subtracted workspace has been generated
  if (m_csSubtractedWS) {
    workspaces.emplace_back(m_csSubtractedWS->getName());
    indices.emplace_back(index);
  }

  m_plotter->plotCorrespondingSpectra(workspaces, indices);
}

/*
 * Plots the selected spectra (selected by the Spectrum spinner) of the
 * specified workspace. The resultant curve will be given the specified name and
 * the specified colour.
 *
 * @param curveName   The name of the curve to plot in the preview.
 * @param ws          The workspace whose spectra to plot in the preview.
 * @param curveColor  The color of the curve to plot in the preview.
 */
void ContainerSubtraction::plotInPreview(const QString &curveName,
                                         MatrixWorkspace_sptr &ws,
                                         const QColor &curveColor) {

  // Check whether the selected spectra is now out of bounds with
  // respect to the specified workspace.
  if (ws->getNumberHistograms() > m_spectra) {
    m_uiForm.ppPreview->addSpectrum(curveName, ws, m_spectra, curveColor);
  } else {
    size_t specNo = 0;

    if (m_csSampleWS) {
      specNo = std::min(ws->getNumberHistograms(),
                        m_csSampleWS->getNumberHistograms()) -
               1;
    } else if (m_csContainerWS) {
      specNo = std::min(ws->getNumberHistograms(),
                        m_csContainerWS->getNumberHistograms()) -
               1;
    }

    m_uiForm.ppPreview->addSpectrum(curveName, ws, specNo, curveColor);
    m_uiForm.spPreviewSpec->setValue(boost::numeric_cast<int>(specNo));
    m_spectra = specNo;
    m_uiForm.spPreviewSpec->setMaximum(boost::numeric_cast<int>(m_spectra));
  }
}

MatrixWorkspace_sptr ContainerSubtraction::requestRebinToSample(
    MatrixWorkspace_sptr workspace) const {
  const char *text =
      "Binning on sample and container does not match."
      "Would you like to rebin the container to match the sample?";

  int result = QMessageBox::question(nullptr, tr("Rebin sample?"), tr(text),
                                     QMessageBox::Yes, QMessageBox::No,
                                     QMessageBox::NoButton);

  if (result == QMessageBox::Yes)
    return rebinToWorkspace(workspace, convertToHistogram(m_csSampleWS));
  else
    return workspace;
}

MatrixWorkspace_sptr
ContainerSubtraction::shiftWorkspace(MatrixWorkspace_sptr workspace,
                                     double shiftValue) const {
  auto shiftAlg = shiftAlgorithm(workspace, shiftValue);
  shiftAlg->execute();
  return shiftAlg->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr
ContainerSubtraction::scaleWorkspace(MatrixWorkspace_sptr workspace,
                                     double scaleValue) const {
  auto scaleAlg = scaleAlgorithm(workspace, scaleValue);
  scaleAlg->execute();
  return scaleAlg->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr
ContainerSubtraction::minusWorkspace(MatrixWorkspace_sptr lhsWorkspace,
                                     MatrixWorkspace_sptr rhsWorkspace) const {
  auto minusAlg = minusAlgorithm(lhsWorkspace, rhsWorkspace);
  minusAlg->execute();
  return minusAlg->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr ContainerSubtraction::rebinToWorkspace(
    MatrixWorkspace_sptr workspaceToRebin,
    MatrixWorkspace_sptr workspaceToMatch) const {
  auto rebinAlg = rebinToWorkspaceAlgorithm(workspaceToRebin, workspaceToMatch);
  rebinAlg->execute();
  return rebinAlg->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr
ContainerSubtraction::convertToHistogram(MatrixWorkspace_sptr workspace) const {
  auto convertAlg = convertToHistogramAlgorithm(workspace);
  convertAlg->execute();
  return convertAlg->getProperty("OutputWorkspace");
}

IAlgorithm_sptr
ContainerSubtraction::shiftAlgorithm(MatrixWorkspace_sptr workspace,
                                     double shiftValue) const {
  IAlgorithm_sptr shift = AlgorithmManager::Instance().create("ScaleX");
  shift->initialize();
  shift->setChild(true);
  shift->setLogging(false);
  shift->setProperty("InputWorkspace", workspace);
  shift->setProperty("Operation", "Add");
  shift->setProperty("Factor", shiftValue);
  shift->setProperty("OutputWorkspace", "shifted");
  return shift;
}

IAlgorithm_sptr
ContainerSubtraction::scaleAlgorithm(MatrixWorkspace_sptr workspace,
                                     double scaleValue) const {
  IAlgorithm_sptr scale = AlgorithmManager::Instance().create("Scale");
  scale->initialize();
  scale->setChild(true);
  scale->setLogging(false);
  scale->setProperty("InputWorkspace", workspace);
  scale->setProperty("Operation", "Multiply");
  scale->setProperty("Factor", scaleValue);
  scale->setProperty("OutputWorkspace", "scaled");
  return scale;
}

IAlgorithm_sptr
ContainerSubtraction::minusAlgorithm(MatrixWorkspace_sptr lhsWorkspace,
                                     MatrixWorkspace_sptr rhsWorkspace) const {
  IAlgorithm_sptr minus = AlgorithmManager::Instance().create("Minus");
  minus->initialize();
  minus->setChild(true);
  minus->setLogging(false);
  minus->setProperty("LHSWorkspace", lhsWorkspace);
  minus->setProperty("RHSWorkspace", rhsWorkspace);
  minus->setProperty("OutputWorkspace", "subtracted");
  return minus;
}

IAlgorithm_sptr ContainerSubtraction::rebinToWorkspaceAlgorithm(
    MatrixWorkspace_sptr workspaceToRebin,
    MatrixWorkspace_sptr workspaceToMatch) const {
  IAlgorithm_sptr rebin =
      AlgorithmManager::Instance().create("RebinToWorkspace");
  rebin->initialize();
  rebin->setChild(true);
  rebin->setLogging(false);
  rebin->setProperty("WorkspaceToRebin", workspaceToRebin);
  rebin->setProperty("WorkspaceToMatch", workspaceToMatch);
  rebin->setProperty("OutputWorkspace", "rebinned");
  return rebin;
}

IAlgorithm_sptr ContainerSubtraction::convertToHistogramAlgorithm(
    MatrixWorkspace_sptr workspace) const {
  IAlgorithm_sptr convert =
      AlgorithmManager::Instance().create("ConvertToHistogram");
  convert->initialize();
  convert->setChild(true);
  convert->setLogging(false);
  convert->setProperty("InputWorkspace", workspace);
  convert->setProperty("OutputWorkspace", "converted");
  return convert;
}

IAlgorithm_sptr ContainerSubtraction::addSampleLogAlgorithm(
    MatrixWorkspace_sptr workspace, const std::string &name,
    const std::string &type, const std::string &value) const {
  IAlgorithm_sptr shiftLog =
      AlgorithmManager::Instance().create("AddSampleLog");
  shiftLog->initialize();
  shiftLog->setProperty("Workspace", workspace);
  shiftLog->setProperty("LogName", name);
  shiftLog->setProperty("LogType", type);
  shiftLog->setProperty("LogText", value);
  return shiftLog;
}

void ContainerSubtraction::setRunEnabled(bool enabled) {
  m_uiForm.pbRun->setEnabled(enabled);
}

void ContainerSubtraction::setSaveResultEnabled(bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
}

void ContainerSubtraction::setButtonsEnabled(bool enabled) {
  setRunEnabled(enabled);
  setSaveResultEnabled(enabled);
}

void ContainerSubtraction::setRunIsRunning(bool running) {
  m_uiForm.pbRun->setText(running ? "Running..." : "Run");
  setButtonsEnabled(!running);
}

} // namespace CustomInterfaces
} // namespace MantidQt
