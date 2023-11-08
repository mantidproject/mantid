// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataAnalysisIqtFitTab.h"
#include "FunctionBrowser/IqtTemplateBrowser.h"
#include "IndirectAddWorkspaceDialog.h"
#include "IndirectFitPlotView.h"
#include "IqtFitModel.h"

#include "MantidAPI/FunctionFactory.h"

using namespace Mantid;
using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("IqtFit");
std::vector<std::string> IQTFIT_HIDDEN_PROPS =
    std::vector<std::string>({"CreateOutput", "LogValue", "PassWSIndexToFunction", "ConvolveMembers",
                              "OutputCompositeMembers", "OutputWorkspace", "Output", "PeakRadius", "PlotParameter"});
} // namespace

namespace MantidQt::CustomInterfaces::IDA {

IndirectDataAnalysisIqtFitTab::IndirectDataAnalysisIqtFitTab(QWidget *parent)
    : IndirectDataAnalysisTab(new IqtFitModel, new IqtTemplateBrowser, IQTFIT_HIDDEN_PROPS, parent) {
  m_uiForm->dockArea->setFitDataView(new IndirectFitDataView(m_uiForm->dockArea));
  setFitDataPresenter(
      std::make_unique<IndirectFitDataPresenter>(m_fittingModel->getFitDataModel(), m_uiForm->dockArea->m_fitDataView));
  setPlotView(m_uiForm->dockArea->m_fitPlotView);
  setOutputOptionsView(m_uiForm->ovOutputOptionsView);

  setEditResultVisible(true);
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

void IndirectDataAnalysisIqtFitTab::addDataToModel(IAddWorkspaceDialog const *dialog) {
  if (const auto indirectDialog = dynamic_cast<IndirectAddWorkspaceDialog const *>(dialog)) {
    m_dataPresenter->addWorkspace(indirectDialog->workspaceName(), indirectDialog->workspaceIndices());
    m_fittingModel->addDefaultParameters();
  }
}

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

} // namespace MantidQt::CustomInterfaces::IDA
