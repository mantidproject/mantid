#include "JumpFit.h"
#include "../General/UserInputValidator.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/lexical_cast.hpp>

#include <string>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

JumpFit::JumpFit(QWidget *parent)
    : IndirectFitAnalysisTab(parent), m_uiForm(new Ui::JumpFit) {
  m_uiForm->setupUi(parent);
  IndirectFitAnalysisTab::addPropertyBrowserToUI(m_uiForm.get());
}

void JumpFit::setup() {
  auto chudleyElliot =
      FunctionFactory::Instance().createFunction("ChudleyElliot");
  auto hallRoss = FunctionFactory::Instance().createFunction("HallRoss");
  auto fickDiffusion =
      FunctionFactory::Instance().createFunction("FickDiffusion");
  auto teixeiraWater =
      FunctionFactory::Instance().createFunction("TeixeiraWater");
  addComboBoxFunctionGroup("ChudleyElliot", {chudleyElliot});
  addComboBoxFunctionGroup("HallRoss", {hallRoss});
  addComboBoxFunctionGroup("FickDiffusion", {fickDiffusion});
  addComboBoxFunctionGroup("TeixeiraWater", {teixeiraWater});

  disablePlotGuess();
  disablePlotPreview();

  // Create range selector
  auto qRangeSelector = m_uiForm->ppPlotTop->addRangeSelector("JumpFitQ");
  connect(qRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(xMinSelected(double)));
  connect(qRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SLOT(xMaxSelected(double)));

  m_uiForm->cbWidth->setEnabled(false);

  // Connect data selector to handler method
  connect(m_uiForm->dsSample, SIGNAL(dataReady(const QString &)), this,
          SLOT(handleSampleInputReady(const QString &)));
  // Connect width selector to handler method
  connect(m_uiForm->cbWidth, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(handleWidthChange(const QString &)));

  // Handle plotting and saving
  connect(m_uiForm->pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm->pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm->pbPlotPreview, SIGNAL(clicked()), this,
          SLOT(plotCurrentPreview()));

  connect(m_uiForm->ckPlotGuess, SIGNAL(stateChanged(int)), this,
          SLOT(updatePlotGuess()));
}

size_t JumpFit::getWidth() const {
  return m_spectraList.at(m_uiForm->cbWidth->currentText().toStdString());
}

int JumpFit::minimumSpectrum() const { return static_cast<int>(getWidth()); }

int JumpFit::maximumSpectrum() const { return static_cast<int>(getWidth()); }

bool JumpFit::doPlotGuess() const {
  return m_uiForm->ckPlotGuess->isEnabled() &&
         m_uiForm->ckPlotGuess->isChecked();
}

/**
 * Validate the form to check the program can be run
 *
 * @return :: Whether the form was valid
 */
bool JumpFit::validate() {
  UserInputValidator uiv;
  uiv.checkDataSelectorIsValid("Sample Input", m_uiForm->dsSample);

  // this workspace doesn't have any valid widths
  if (m_spectraList.empty())
    uiv.addErrorMessage(
        "Sample Input: Workspace doesn't appear to contain any width data");

  if (isEmptyModel())
    uiv.addErrorMessage("No fit function has been selected");

  const auto errors = uiv.generateErrorMessage();
  emit showMessageBox(errors);
  return errors.isEmpty();
}

/**
 * Handles the JumpFit algorithm finishing, used to plot fit in miniplot.
 *
 * @param error True if the algorithm failed, false otherwise
 */
void JumpFit::algorithmComplete(bool error) {
  // Ignore errors
  if (error)
    return;
  m_uiForm->pbPlot->setEnabled(true);
  m_uiForm->pbSave->setEnabled(true);

  // Process the parameters table
  const auto paramWsName = outputWorkspaceName() + "_Parameters";
  IndirectFitAnalysisTab::fitAlgorithmComplete(paramWsName);
}

/**
 * Set the data selectors to use the default save directory
 * when browsing for input files.
 *
 * @param settings :: The current settings
 */
void JumpFit::loadSettings(const QSettings &settings) {
  m_uiForm->dsSample->readSettings(settings.group());
}

/**
 * Plots the loaded file to the miniplot and sets the guides
 * and the range
 *
 * @param filename :: The name of the workspace to plot
 */
void JumpFit::handleSampleInputReady(const QString &filename) {
  // Scale to convert to HWHM
  const auto sample = filename + "_HWHM";
  scaleAlgorithm(filename.toStdString(), sample.toStdString(), 0.5)->execute();

  IndirectFitAnalysisTab::newInputDataLoaded(sample);

  QPair<double, double> res;
  QPair<double, double> range = m_uiForm->ppPlotTop->getCurveRange("Sample");
  auto bounds = getResolutionRangeFromWs(sample, res) ? res : range;
  auto qRangeSelector = m_uiForm->ppPlotTop->getRangeSelector("JumpFitQ");
  qRangeSelector->setMinimum(bounds.first);
  qRangeSelector->setMaximum(bounds.second);

  findAllWidths(inputWorkspace());

  if (m_spectraList.size() > 0) {
    m_uiForm->cbWidth->setEnabled(true);
    const auto width = static_cast<int>(getWidth());
    setMinimumSpectrum(width);
    setMaximumSpectrum(width);
    setSelectedSpectrum(width);
  } else {
    m_uiForm->cbWidth->setEnabled(false);
    emit showMessageBox("Workspace doesn't appear to contain any width data");
  }
}

/**
 * Find all of the spectra in the workspace that have width data
 *
 * @param ws :: The workspace to search
 */
void JumpFit::findAllWidths(MatrixWorkspace_const_sptr ws) {
  MantidQt::API::SignalBlocker<QObject> blocker(m_uiForm->cbWidth);
  m_uiForm->cbWidth->clear();

  auto axis = dynamic_cast<TextAxis *>(ws->getAxis(1));

  if (axis) {
    m_spectraList = findAxisLabelsWithSubstrings(axis, {".Width", ".FWHM"}, 3);

    for (const auto &iter : m_spectraList)
      m_uiForm->cbWidth->addItem(QString::fromStdString(iter.first));
  } else
    m_spectraList.clear();
}

std::map<std::string, size_t> JumpFit::findAxisLabelsWithSubstrings(
    TextAxis *axis, const std::vector<std::string> &substrings,
    const size_t &maximumNumber) const {
  std::map<std::string, size_t> labels;

  for (size_t i = 0u; i < axis->length(); ++i) {
    const auto label = axis->label(i);
    size_t substringIndex = 0;
    size_t foundIndex = std::string::npos;

    while (substringIndex < substrings.size() &&
           foundIndex == std::string::npos && labels.size() < maximumNumber)
      foundIndex = label.find(substrings[substringIndex++]);

    if (foundIndex != std::string::npos)
      labels[label] = i;
  }
  return labels;
}

/**
 * Plots the loaded file to the miniplot when the selected spectrum changes
 *
 * @param text :: The name spectrum index to plot
 */
void JumpFit::handleWidthChange(const QString &text) {
  const auto width = text.toStdString();

  if (m_spectraList.find(width) != m_spectraList.end())
    setSelectedSpectrum(static_cast<int>(m_spectraList[width]));
}

void JumpFit::startXChanged(double startX) {
  auto rangeSelector = m_uiForm->ppPlotTop->getRangeSelector("JumpFitQ");
  MantidQt::API::SignalBlocker<QObject> blocker(rangeSelector);
  rangeSelector->setMinimum(startX);
}

void JumpFit::endXChanged(double endX) {
  auto rangeSelector = m_uiForm->ppPlotTop->getRangeSelector("JumpFitQ");
  MantidQt::API::SignalBlocker<QObject> blocker(rangeSelector);
  rangeSelector->setMaximum(endX);
}

void JumpFit::disablePlotGuess() { m_uiForm->ckPlotGuess->setEnabled(false); }

void JumpFit::enablePlotGuess() { m_uiForm->ckPlotGuess->setEnabled(true); }

/**
 * Updates the plot
 */
void JumpFit::updatePreviewPlots() {
  IndirectFitAnalysisTab::updatePlots(m_uiForm->ppPlotTop,
                                      m_uiForm->ppPlotBottom);
}

void JumpFit::updatePlotRange() {
  auto rangeSelector = m_uiForm->ppPlotTop->getRangeSelector("JumpFitQ");
  if (m_uiForm->ppPlotTop->hasCurve("Sample")) {
    const auto range = m_uiForm->ppPlotTop->getCurveRange("Sample");
    rangeSelector->setRange(range.first, range.second);
  }
}

std::string JumpFit::createSingleFitOutputName() const {
  auto outputName = inputWorkspace()->getName();
  auto position = outputName.rfind("_Result");

  if (position != std::string::npos)
    outputName = outputName.substr(0, position) +
                 outputName.substr(position + 7, outputName.size());
  return outputName + "_" + selectedFitType().toStdString() + "_JumpFit";
}

IAlgorithm_sptr JumpFit::singleFitAlgorithm() const {
  const auto widthText = m_uiForm->cbWidth->currentText().toStdString();
  const auto width = m_spectraList.at(widthText);

  auto fitAlg = AlgorithmManager::Instance().create("QENSFitSequential");
  fitAlg->initialize();
  fitAlg->setProperty("SpecMin", static_cast<int>(width));
  fitAlg->setProperty("SpecMax", static_cast<int>(width));
  fitAlg->setProperty("OutputWorkspace",
                      createSingleFitOutputName() + "_Result");
  return fitAlg;
}

IAlgorithm_sptr
JumpFit::deleteWorkspaceAlgorithm(const std::string &workspaceName) {
  auto deleteAlg = AlgorithmManager::Instance().create("DeleteWorkspace");
  deleteAlg->setProperty("Workspace", workspaceName);
  return deleteAlg;
}

IAlgorithm_sptr JumpFit::scaleAlgorithm(const std::string &workspaceToScale,
                                        const std::string &outputName,
                                        double scaleFactor) {
  auto scaleAlg = AlgorithmManager::Instance().create("Scale");
  scaleAlg->initialize();
  scaleAlg->setProperty("InputWorkspace", workspaceToScale);
  scaleAlg->setProperty("OutputWorkspace", outputName);
  scaleAlg->setProperty("Factor", scaleFactor);
  return scaleAlg;
}

void JumpFit::updatePlotOptions() {}

void JumpFit::enablePlotResult() { m_uiForm->pbPlot->setEnabled(true); }

void JumpFit::disablePlotResult() { m_uiForm->pbPlot->setEnabled(false); }

void JumpFit::enableSaveResult() { m_uiForm->pbSave->setEnabled(true); }

void JumpFit::disableSaveResult() { m_uiForm->pbSave->setEnabled(false); }

void JumpFit::enablePlotPreview() { m_uiForm->pbPlotPreview->setEnabled(true); }

void JumpFit::disablePlotPreview() {
  m_uiForm->pbPlotPreview->setEnabled(false);
}

void JumpFit::addGuessPlot(MatrixWorkspace_sptr workspace) {
  m_uiForm->ppPlotTop->addSpectrum("Guess", workspace, 0, Qt::green);
}

void JumpFit::removeGuessPlot() {
  m_uiForm->ppPlotTop->removeSpectrum("Guess");
  m_uiForm->ckPlotGuess->setChecked(false);
}

/**
 * Handles mantid plotting
 */
void JumpFit::plotClicked() {
  const auto outWsName = outputWorkspaceName() + "_Workspace";
  IndirectFitAnalysisTab::plotResult(outWsName, "All");
}

/**
 * Handles saving of workspace
 */
void JumpFit::saveClicked() {
  const auto outWsName = outputWorkspaceName() + "_Workspace";
  IndirectFitAnalysisTab::saveResult(outWsName);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
