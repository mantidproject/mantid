// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataAnalysisFqFitTab.h"
#include "FQFitConstants.h"
#include "FqFitDataPresenter.h"
#include "FqFitModel.h"

#include "FunctionBrowser/FqTemplateBrowser.h"

#include <string>

using namespace Mantid::API;

namespace {

std::vector<std::string> FQFIT_HIDDEN_PROPS =
    std::vector<std::string>({"CreateOutput", "LogValue", "PassWSIndexToFunction", "ConvolveMembers",
                              "OutputCompositeMembers", "OutputWorkspace", "Output", "PeakRadius", "PlotParameter"});

} // namespace

namespace MantidQt::CustomInterfaces::IDA {

IndirectDataAnalysisFqFitTab::IndirectDataAnalysisFqFitTab(QWidget *parent)
    : IndirectDataAnalysisTab(new FqFitModel, new FqTemplateBrowser, new FqFitDataView, FQFIT_HIDDEN_PROPS, parent) {
  // m_fitPropertyBrowser->updateAvailableFunctions(availableFits.at(DataType::ALL));

  setupFitDataPresenter<FqFitDataPresenter>();
  m_plotPresenter->setXBounds({0.0, 2.0});
}

EstimationDataSelector IndirectDataAnalysisFqFitTab::getEstimationDataSelector() const {
  return [](const std::vector<double> &x, const std::vector<double> &y,
            const std::pair<double, double> range) -> DataForParameterEstimation {
    // Find data thats within range
    double xmin = range.first;
    double xmax = range.second;

    // If the two points are equal return empty data
    if (fabs(xmin - xmax) < 1e-7) {
      return DataForParameterEstimation{};
    }

    const auto startItr =
        std::find_if(x.cbegin(), x.cend(), [xmin](const double &val) -> bool { return val >= (xmin - 1e-7); });
    auto endItr = std::find_if(x.cbegin(), x.cend(), [xmax](const double &val) -> bool { return val > xmax; });

    if (std::distance(startItr, endItr - 1) < 2)
      return DataForParameterEstimation{};

    size_t first = std::distance(x.cbegin(), startItr);
    size_t end = std::distance(x.cbegin(), endItr);
    size_t m = first + (end - first) / 2;

    return DataForParameterEstimation{{x[first], x[m]}, {y[first], y[m]}};
  };
}

} // namespace MantidQt::CustomInterfaces::IDA
