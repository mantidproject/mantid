// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "JumpFit.h"
#include "JumpFitDataPresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <boost/algorithm/string/join.hpp>

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
      m_sampleFBExtensions({"_Result.nxs"}), m_sampleWSExtensions({"_Result"}),
      m_uiForm(new Ui::JumpFit) {
  m_uiForm->setupUi(parent);

  m_jumpFittingModel = dynamic_cast<JumpFitModel *>(fittingModel());
  setFitDataPresenter(Mantid::Kernel::make_unique<JumpFitDataPresenter>(
      m_jumpFittingModel, m_uiForm->fitDataView, m_uiForm->cbParameterType,
      m_uiForm->cbParameter, m_uiForm->lbParameterType, m_uiForm->lbParameter));
  setPlotView(m_uiForm->pvFitPlotView);
  setSpectrumSelectionView(m_uiForm->svSpectrumView);
  setOutputOptionsView(m_uiForm->ovOutputOptionsView);
  setFitPropertyBrowser(m_uiForm->fitPropertyBrowser);

  setEditResultVisible(false);
}

void JumpFit::setupFitTab() {
  m_uiForm->svSpectrumView->hideSpectrumSelector();
  m_uiForm->svSpectrumView->hideMaskSpectrumSelector();

  setSampleFBSuffices(m_sampleFBExtensions);
  setSampleWSSuffices(m_sampleWSExtensions);

  addFunctions(getWidthFunctions());
  addFunctions(getEISFFunctions());

  m_uiForm->cbParameter->setEnabled(false);

  // Handle plotting and saving
  connect(m_uiForm->pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(this, SIGNAL(functionChanged()), this,
          SLOT(updateModelFitTypeString()));
  connect(m_uiForm->cbParameterType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updateAvailableFitTypes()));
  connect(this, SIGNAL(updateAvailableFitTypes()), this,
          SLOT(updateAvailableFitTypes()));
}

QStringList JumpFit::getSampleFBSuffices() const {
  return m_sampleFBExtensions;
}

QStringList JumpFit::getSampleWSSuffices() const {
  return m_sampleWSExtensions;
}

QStringList JumpFit::getResolutionFBSuffices() const { return {}; }

QStringList JumpFit::getResolutionWSSuffices() const { return {}; }

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

void JumpFit::runClicked() { runTab(); }

void JumpFit::setRunIsRunning(bool running) {
  m_uiForm->pbRun->setText(running ? "Running..." : "Run");
}

void JumpFit::setRunEnabled(bool enable) {
  m_uiForm->pbRun->setEnabled(enable);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
