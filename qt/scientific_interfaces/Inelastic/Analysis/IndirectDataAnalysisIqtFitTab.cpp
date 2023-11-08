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

using namespace Mantid::API;

namespace {
std::vector<std::string> IQTFIT_HIDDEN_PROPS =
    std::vector<std::string>({"CreateOutput", "LogValue", "PassWSIndexToFunction", "ConvolveMembers",
                              "OutputCompositeMembers", "OutputWorkspace", "Output", "PeakRadius", "PlotParameter"});
} // namespace

namespace MantidQt::CustomInterfaces::IDA {

IndirectDataAnalysisIqtFitTab::IndirectDataAnalysisIqtFitTab(QWidget *parent)
    : IndirectDataAnalysisTab(new IqtFitModel, new IqtTemplateBrowser, new IndirectFitDataView, IQTFIT_HIDDEN_PROPS,
                              parent) {}

EstimationDataSelector IndirectDataAnalysisIqtFitTab::getEstimationDataSelector() const {
  return [](const Mantid::MantidVec &x, const Mantid::MantidVec &y,
            const std::pair<double, double> range) -> DataForParameterEstimation {
    (void)range;
    size_t const n = 4;
    if (y.size() < n + 1)
      return DataForParameterEstimation{{}, {}};
    return DataForParameterEstimation{{x[0], x[n]}, {y[0], y[n]}};
  };
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
