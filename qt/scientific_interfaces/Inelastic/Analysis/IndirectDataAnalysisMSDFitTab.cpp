// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataAnalysisMSDFitTab.h"
#include "IDAFunctionParameterEstimation.h"
#include "MSDFitModel.h"

#include "FunctionBrowser/SingleFunctionTemplateBrowser.h"

using namespace Mantid::API;

namespace {
std::vector<std::string> MSDFIT_HIDDEN_PROPS =
    std::vector<std::string>({"CreateOutput", "LogValue", "PassWSIndexToFunction", "ConvolveMembers",
                              "OutputCompositeMembers", "OutputWorkspace", "Output", "PeakRadius", "PlotParameter"});

const std::string MSDGAUSSFUNC{"MsdGauss"};
const std::string MSDPETERSFUNC{"MsdPeters"};
const std::string MSDYIFUNC{"MsdYi"};

auto MSD_FUNCTION_STRINGS =
    std::map<std::string, std::string>({{"None", ""},
                                        {"Gauss", "name=MsdGauss,Height=1,Msd=0.05,constraints=(Height>0, Msd>0)"},
                                        {"Peters", "name=MsdPeters,Height=1,Msd=0.05,Beta=1,constraints=(Height>0, "
                                                   "Msd>0, Beta>0)"},
                                        {"Yi", "name=MsdYi,Height=1,Msd=0.05,Sigma=1,constraints=(Height>0, Msd>0, "
                                               "Sigma>0)"}});

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectDataAnalysisMSDFitTab::IndirectDataAnalysisMSDFitTab(QWidget *parent)
    : IndirectDataAnalysisTab(
          new MSDFitModel,
          new SingleFunctionTemplateBrowser(
              MSD_FUNCTION_STRINGS, std::make_unique<IDAFunctionParameterEstimation>(createParameterEstimation())),
          new IndirectFitDataView, MSDFIT_HIDDEN_PROPS, parent) {
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

// Create parameter estimation functions
// These function rely on the data returned from getEstimationDataSelector,
// which should be appropriately configured.
IDAFunctionParameterEstimation IndirectDataAnalysisMSDFitTab::createParameterEstimation() const {
  auto estimateMsd = [](::Mantid::API::IFunction_sptr &function, const DataForParameterEstimation &estimationData) {
    auto y = estimationData.y;
    auto x = estimationData.x;
    if (x.size() != 2 || y.size() != 2) {
      return;
    }
    double Msd = 6 * log(y[0] / y[1]) / (x[1] * x[1]);
    // If MSD less than zero, reject the estimate and set to default value of
    // 0.05, which leads to a (roughly) flat line
    Msd = Msd > 0 ? Msd : 0.05;
    function->setParameter("Msd", Msd);
    function->setParameter("Height", y[0]);
  };
  IDAFunctionParameterEstimation parameterEstimation;

  parameterEstimation.addParameterEstimationFunction(MSDGAUSSFUNC, estimateMsd);
  parameterEstimation.addParameterEstimationFunction(MSDPETERSFUNC, estimateMsd);
  parameterEstimation.addParameterEstimationFunction(MSDYIFUNC, estimateMsd);

  return parameterEstimation;
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
