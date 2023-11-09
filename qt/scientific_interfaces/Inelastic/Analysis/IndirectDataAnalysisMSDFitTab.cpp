// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataAnalysisMSDFitTab.h"
#include "MSDFitModel.h"

#include "FunctionBrowser/MSDTemplateBrowser.h"

using namespace Mantid::API;

namespace {
std::vector<std::string> MSDFIT_HIDDEN_PROPS =
    std::vector<std::string>({"CreateOutput", "LogValue", "PassWSIndexToFunction", "ConvolveMembers",
                              "OutputCompositeMembers", "OutputWorkspace", "Output", "PeakRadius", "PlotParameter"});

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectDataAnalysisMSDFitTab::IndirectDataAnalysisMSDFitTab(QWidget *parent)
    : IndirectDataAnalysisTab(new MSDFitModel, new MSDTemplateBrowser, new IndirectFitDataView, MSDFIT_HIDDEN_PROPS,
                              parent) {
  setupFitDataPresenter<IndirectFitDataPresenter>();
}

EstimationDataSelector IndirectDataAnalysisMSDFitTab::getEstimationDataSelector() const {

  return [](const std::vector<double> &x, const std::vector<double> &y,
            const std::pair<double, double> range) -> DataForParameterEstimation {
    // Find data thats within range
    double xmin = range.first;
    double xmax = range.second;
    if (xmin > xmax) {
      return DataForParameterEstimation{};
    }

    const auto startItr =
        std::find_if(x.cbegin(), x.cend(), [xmin](const double &val) -> bool { return val >= (xmin - 1e-5); });
    auto endItr = std::find_if(x.cbegin(), x.cend(), [xmax](const double &val) -> bool { return val > xmax; });

    size_t first = std::distance(x.cbegin(), startItr);
    size_t end = std::distance(x.cbegin(), endItr);
    size_t m = first + (end - first) / 2;

    if (std::distance(startItr, endItr - 1) < 2)
      return DataForParameterEstimation{};

    return DataForParameterEstimation{{x[first], x[m]}, {y[first], y[m]}};
  };
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
