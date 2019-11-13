// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IqtFit.h"
#include "IndirectFunctionBrowser/IqtTemplateBrowser.h"

#include "MantidQtWidgets/Common/UserInputValidator.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <QMenu>

#include <boost/algorithm/string/predicate.hpp>

using namespace Mantid;
using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("IqtFit");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IqtFit::IqtFit(QWidget *parent)
    : IndirectFitAnalysisTab(new IqtFitModel, parent),
      m_uiForm(new Ui::IqtFit) {
  m_uiForm->setupUi(parent);
  m_iqtFittingModel = dynamic_cast<IqtFitModel *>(fittingModel());
  setFitDataPresenter(std::make_unique<IndirectFitDataPresenter>(
      m_iqtFittingModel, m_uiForm->fitDataView));
  setPlotView(m_uiForm->pvFitPlotView);
  setSpectrumSelectionView(m_uiForm->svSpectrumView);
  setOutputOptionsView(m_uiForm->ovOutputOptionsView);
  auto templateBrowser = new IqtTemplateBrowser;
  m_uiForm->fitPropertyBrowser->setFunctionTemplateBrowser(templateBrowser);
  setFitPropertyBrowser(m_uiForm->fitPropertyBrowser);

  setEditResultVisible(true);
  setStartAndEndHidden(false);
}

void IqtFit::setupFitTab() {
  // Create custom function groups
  auto &functionFactory = FunctionFactory::Instance();
  const auto exponential = functionFactory.createFunction("ExpDecay");
  const auto stretchedExponential =
      functionFactory.createFunction("StretchExp");

  connect(m_uiForm->pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(this, SIGNAL(functionChanged()), this, SLOT(fitFunctionChanged()));
}

EstimationDataSelector IqtFit::getEstimationDataSelector() const {
  return
      [](const MantidVec &x, const MantidVec &y) -> DataForParameterEstimation {
        size_t const n = 4;
        if (y.size() < n + 1)
          return DataForParameterEstimation{{}, {}};
        return DataForParameterEstimation{{x[0], x[n]}, {y[0], y[n]}};
      };
}

void IqtFit::fitFunctionChanged() {
  m_iqtFittingModel->setFitTypeString(fitTypeString());
}

std::string IqtFit::fitTypeString() const {
  const auto numberOfExponential = numberOfCustomFunctions("ExpDecay");
  const auto numberOfStretched = numberOfCustomFunctions("StretchExp");

  if (numberOfExponential > 0)
    return std::to_string(numberOfExponential) + "E";

  if (numberOfStretched > 0)
    return std::to_string(numberOfStretched) + "S";

  return "";
}

void IqtFit::setupFit(Mantid::API::IAlgorithm_sptr fitAlgorithm) {
  IndirectFitAnalysisTab::setupFit(fitAlgorithm);
}

void IqtFit::runClicked() { runTab(); }

void IqtFit::setRunIsRunning(bool running) {
  m_uiForm->pbRun->setText(running ? "Running..." : "Run");
}

void IqtFit::setRunEnabled(bool enable) { m_uiForm->pbRun->setEnabled(enable); }

void IqtFit::setStartAndEndHidden(bool hidden) {
  m_uiForm->fitDataView->setStartAndEndHidden(hidden);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
