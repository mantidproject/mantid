// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataAnalysisIqtFitTab.h"
#include "IndirectFitPlotView.h"
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
std::vector<std::string> IQTFIT_HIDDEN_PROPS = std::vector<std::string>(
    {"CreateOutput", "LogValue", "PassWSIndexToFunction", "ConvolveMembers", "OutputCompositeMembers",
     "OutputWorkspace", "IgnoreInvalidData", "Output", "PeakRadius", "PlotParameter"});
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectDataAnalysisIqtFitTab::IndirectDataAnalysisIqtFitTab(QWidget *parent)
    : IndirectFitAnalysisTab(new IqtFitModel, parent), m_uiForm(new Ui::IndirectFitTab) {
  m_uiForm->setupUi(parent);
  m_iqtFittingModel = dynamic_cast<IqtFitModel *>(getFittingModel());
  setFitDataPresenter(std::make_unique<IndirectFitDataPresenter>(m_iqtFittingModel, m_uiForm->dockArea->m_fitDataView));
  setPlotView(m_uiForm->dockArea->m_fitPlotView);
  setOutputOptionsView(m_uiForm->ovOutputOptionsView);
  auto templateBrowser = new IqtTemplateBrowser;
  m_uiForm->dockArea->m_fitPropertyBrowser->setFunctionTemplateBrowser(templateBrowser);
  setFitPropertyBrowser(m_uiForm->dockArea->m_fitPropertyBrowser);
  m_uiForm->dockArea->m_fitPropertyBrowser->setHiddenProperties(IQTFIT_HIDDEN_PROPS);

  setEditResultVisible(true);
}

void IndirectDataAnalysisIqtFitTab::setupFitTab() {
  // Create custom function groups
  auto &functionFactory = FunctionFactory::Instance();
  const auto exponential = functionFactory.createFunction("ExpDecay");
  const auto stretchedExponential = functionFactory.createFunction("StretchExp");

  connect(m_uiForm->pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(this, SIGNAL(functionChanged()), this, SLOT(fitFunctionChanged()));
}

EstimationDataSelector IndirectDataAnalysisIqtFitTab::getEstimationDataSelector() const {
  return
      [](const MantidVec &x, const MantidVec &y, const std::pair<double, double> range) -> DataForParameterEstimation {
        (void)range;
        size_t const n = 4;
        if (y.size() < n + 1)
          return DataForParameterEstimation{{}, {}};
        return DataForParameterEstimation{{x[0], x[n]}, {y[0], y[n]}};
      };
}

void IndirectDataAnalysisIqtFitTab::fitFunctionChanged() { m_iqtFittingModel->setFitTypeString(getFitTypeString()); }

std::string IndirectDataAnalysisIqtFitTab::getFitTypeString() const {
  const auto numberOfExponential = getNumberOfCustomFunctions("ExpDecay");
  const auto numberOfStretched = getNumberOfCustomFunctions("StretchExp");
  std::string functionType{""};
  if (numberOfExponential > 0)
    functionType += std::to_string(numberOfExponential) + "E";

  if (numberOfStretched > 0)
    functionType += std::to_string(numberOfStretched) + "S";

  return functionType;
}

void IndirectDataAnalysisIqtFitTab::setupFit(Mantid::API::IAlgorithm_sptr fitAlgorithm) {
  IndirectFitAnalysisTab::setupFit(fitAlgorithm);
}

void IndirectDataAnalysisIqtFitTab::runClicked() { runTab(); }

void IndirectDataAnalysisIqtFitTab::setRunIsRunning(bool running) {
  m_uiForm->pbRun->setText(running ? "Running..." : "Run");
}

void IndirectDataAnalysisIqtFitTab::setRunEnabled(bool enable) { m_uiForm->pbRun->setEnabled(enable); }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
