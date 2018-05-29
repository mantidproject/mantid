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
    : IndirectFitAnalysisTab(new JumpFitModel, parent),
      m_uiForm(new Ui::JumpFit) {
  m_uiForm->setupUi(parent);
}

void JumpFit::setupFitTab() {
  m_jumpFittingModel = dynamic_cast<JumpFitModel *>(fittingModel());
  setFitPropertyBrowser(m_uiForm->fitPropertyBrowser);
  setSpectrumSelectionView(m_uiForm->svSpectrumView);

  m_uiForm->svSpectrumView->hideSpectrumSelector();
  m_uiForm->svSpectrumView->hideMaskSpectrumSelector();

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
  connect(m_uiForm->cbWidth, SIGNAL(currentIndexChanged(int)), this,
          SLOT(handleWidthChange(int)));

  // Handle plotting and saving
  connect(m_uiForm->pbSave, SIGNAL(clicked()), this, SLOT(saveResult()));
  connect(m_uiForm->pbPlotPreview, SIGNAL(clicked()), this,
          SLOT(plotCurrentPreview()));

  connect(m_uiForm->ckPlotGuess, SIGNAL(stateChanged(int)), this,
          SLOT(updatePlotGuess()));
  connect(this, SIGNAL(functionChanged()), this,
          SLOT(updateModelFitTypeString()));
}

void JumpFit::updateModelFitTypeString() {
  m_jumpFittingModel->setFitType(selectedFitType().toStdString());
}

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
  if (m_jumpFittingModel->getWidths(0).empty())
    uiv.addErrorMessage(
        "Sample Input: Workspace doesn't appear to contain any width data");

  if (isEmptyModel())
    uiv.addErrorMessage("No fit function has been selected");

  const auto errors = uiv.generateErrorMessage();
  emit showMessageBox(errors);
  return errors.isEmpty();
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
  IndirectFitAnalysisTab::newInputDataLoaded(filename);
  setAvailableWidths(m_jumpFittingModel->getWidths(0));

  QPair<double, double> res;
  QPair<double, double> range = m_uiForm->ppPlotTop->getCurveRange("Sample");
  auto bounds = getResolutionRangeFromWs(filename, res) ? res : range;
  auto qRangeSelector = m_uiForm->ppPlotTop->getRangeSelector("JumpFitQ");
  qRangeSelector->setMinimum(bounds.first);
  qRangeSelector->setMaximum(bounds.second);
  const auto width = m_jumpFittingModel->getWidthSpectrum(0, 0);

  if (width) {
    m_uiForm->cbWidth->setEnabled(true);
    setMinimumSpectrum(static_cast<int>(*width));
    setMaximumSpectrum(static_cast<int>(*width));
    setSelectedSpectrum(static_cast<int>(*width));
  } else {
    m_uiForm->cbWidth->setEnabled(false);
    emit showMessageBox("Workspace doesn't appear to contain any width data");
  }
}

void JumpFit::setAvailableWidths(const std::vector<std::string> &widths) {
  MantidQt::API::SignalBlocker<QObject> blocker(m_uiForm->cbWidth);
  m_uiForm->cbWidth->clear();
  for (const auto &width : widths)
    m_uiForm->cbWidth->addItem(QString::fromStdString(width));
}

/**
 * Plots the loaded file to the miniplot when the selected spectrum changes
 *
 * @param text :: The name spectrum index to plot
 */
void JumpFit::handleWidthChange(int widthIndex) {
  auto index = static_cast<std::size_t>(widthIndex);
  auto spectrum = m_jumpFittingModel->getWidthSpectrum(index, 0);

  if (spectrum) {
    m_jumpFittingModel->setActiveWidth(index, 0);
    setSelectedSpectrum(static_cast<int>(*spectrum));
  }
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

void JumpFit::updatePlotOptions() {}

void JumpFit::setPlotResultEnabled(bool enabled) {}

void JumpFit::setSaveResultEnabled(bool enabled) {
  m_uiForm->pbSave->setEnabled(enabled);
}

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

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
