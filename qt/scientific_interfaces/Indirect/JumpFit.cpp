// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "JumpFit.h"
#include "IndirectFunctionBrowser/FQTemplateBrowser.h"
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

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

JumpFit::JumpFit(QWidget *parent)
    : IndirectFitAnalysisTab(new JumpFitModel, parent),
      m_uiForm(new Ui::JumpFit) {
  m_uiForm->setupUi(parent);

  m_jumpFittingModel = dynamic_cast<JumpFitModel *>(fittingModel());
  auto templateBrowser = new FQTemplateBrowser;
  setPlotView(m_uiForm->pvFitPlotView);
  setFitDataPresenter(std::make_unique<JumpFitDataPresenter>(
      m_jumpFittingModel, m_uiForm->fitDataView, m_uiForm->cbParameterType,
      m_uiForm->cbParameter, m_uiForm->lbParameterType, m_uiForm->lbParameter,
      templateBrowser));
  connect(m_dataPresenter.get(), SIGNAL(spectrumChanged(WorkspaceIndex)),
          m_plotPresenter.get(),
          SLOT(handlePlotSpectrumChanged(WorkspaceIndex)));
  setSpectrumSelectionView(m_uiForm->svSpectrumView);
  setOutputOptionsView(m_uiForm->ovOutputOptionsView);

  m_uiForm->fitPropertyBrowser->setFunctionTemplateBrowser(templateBrowser);
  setFitPropertyBrowser(m_uiForm->fitPropertyBrowser);

  setEditResultVisible(false);
  m_uiForm->fitDataView->setStartAndEndHidden(false);
}

void JumpFit::setupFitTab() {
  m_uiForm->svSpectrumView->hideSpectrumSelector();
  m_uiForm->svSpectrumView->hideMaskSpectrumSelector();

  m_uiForm->cbParameter->setEnabled(false);

  connect(m_uiForm->pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm->cbParameterType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updateAvailableFitTypes()));
  connect(this, SIGNAL(updateAvailableFitTypes()), this,
          SLOT(updateAvailableFitTypes()));
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

EstimationDataSelector JumpFit::getEstimationDataSelector() const {
  return [](const std::vector<double> &,
            const std::vector<double> &) -> DataForParameterEstimation {
    return DataForParameterEstimation{};
  };
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
