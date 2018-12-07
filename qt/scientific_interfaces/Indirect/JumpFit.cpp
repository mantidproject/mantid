// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "JumpFit.h"
#include "JumpFitDataPresenter.h"

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

namespace {

std::vector<std::string> getEISFFunctions() {
  return {"EISFDiffCylinder", "EISFDiffSphere", "EISFDiffSphereAlkyl"};
}

std::vector<std::string> getWidthFunctions() {
  return {"ChudleyElliot", "HallRoss", "FickDiffusion", "TeixeiraWater"};
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

JumpFit::JumpFit(QWidget *parent)
    : IndirectFitAnalysisTab(new JumpFitModel, parent),
      m_uiForm(new Ui::JumpFit) {
  m_uiForm->setupUi(parent);

  m_jumpFittingModel = dynamic_cast<JumpFitModel *>(fittingModel());
  setFitDataPresenter(Mantid::Kernel::make_unique<JumpFitDataPresenter>(
      m_jumpFittingModel, m_uiForm->fitDataView, m_uiForm->cbParameterType,
      m_uiForm->cbParameter, m_uiForm->lbParameterType, m_uiForm->lbParameter));
  setPlotView(m_uiForm->pvFitPlotView);
  setSpectrumSelectionView(m_uiForm->svSpectrumView);
  setFitPropertyBrowser(m_uiForm->fitPropertyBrowser);
}

void JumpFit::setupFitTab() {
  m_uiForm->svSpectrumView->hideSpectrumSelector();
  m_uiForm->svSpectrumView->hideMaskSpectrumSelector();

  setSampleWSSuffices({"_Result"});
  setSampleFBSuffices({"_Result.nxs"});

  addFunctions(getWidthFunctions());
  addFunctions(getEISFFunctions());

  m_uiForm->cbParameter->setEnabled(false);

  // Handle plotting and saving
  connect(m_uiForm->pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm->pbSave, SIGNAL(clicked()), this, SLOT(saveResult()));
  connect(m_uiForm->pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(this, SIGNAL(functionChanged()), this,
          SLOT(updateModelFitTypeString()));
  connect(m_uiForm->cbParameterType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updateAvailableFitTypes()));
}

void JumpFit::updateAvailableFitTypes() {
  auto const parameter = m_uiForm->cbParameterType->currentText().toStdString();
  clearFitTypeComboBox();
  if (parameter == "Width")
    addFunctions(getWidthFunctions());
  else if (parameter == "EISF")
    addFunctions(getEISFFunctions());
}

void JumpFit::addFunctions(std::vector<std::string> const &functions) {
  auto &factory = FunctionFactory::Instance();
  for (auto const &function : functions)
    addComboBoxFunctionGroup(QString::fromStdString(function),
                             {factory.createFunction(function)});
}

void JumpFit::updateModelFitTypeString() {
  m_jumpFittingModel->setFitType(selectedFitType().toStdString());
}

void JumpFit::updatePlotOptions() {
  IndirectFitAnalysisTab::updatePlotOptions(m_uiForm->cbPlotType);
}

void JumpFit::runClicked() { runTab(); }

void JumpFit::plotClicked() {
  setPlotResultIsPlotting(true);
  IndirectFitAnalysisTab::plotResult(m_uiForm->cbPlotType->currentText());
  setPlotResultIsPlotting(false);
}

void JumpFit::setRunIsRunning(bool running) {
  m_uiForm->pbRun->setText(running ? "Running..." : "Run");
  setButtonsEnabled(!running);
}

void JumpFit::setFitSingleSpectrumIsFitting(bool fitting) {
  m_uiForm->pvFitPlotView->setFitSingleSpectrumText(
      fitting ? "Fitting..." : "Fit Single Spectrum");
  setButtonsEnabled(!fitting);
}

void JumpFit::setPlotResultIsPlotting(bool plotting) {
  m_uiForm->pbPlot->setText(plotting ? "Plotting..." : "Plot");
  setButtonsEnabled(!plotting);
}

void JumpFit::setButtonsEnabled(bool enabled) {
  setRunEnabled(enabled);
  setPlotResultEnabled(enabled);
  setSaveResultEnabled(enabled);
  setFitSingleSpectrumEnabled(enabled);
}

void JumpFit::setRunEnabled(bool enabled) {
  m_uiForm->pbRun->setEnabled(enabled);
}

void JumpFit::setPlotResultEnabled(bool enabled) {
  m_uiForm->pbPlot->setEnabled(enabled);
  m_uiForm->cbPlotType->setEnabled(enabled);
}

void JumpFit::setFitSingleSpectrumEnabled(bool enabled) {
  m_uiForm->pvFitPlotView->enableFitSingleSpectrum(enabled);
}

void JumpFit::setSaveResultEnabled(bool enabled) {
  m_uiForm->pbSave->setEnabled(enabled);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
