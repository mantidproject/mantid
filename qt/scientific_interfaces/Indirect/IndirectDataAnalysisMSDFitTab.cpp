// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataAnalysisMSDFitTab.h"
#include "IDAFunctionParameterEstimation.h"

#include "IndirectFunctionBrowser/SingleFunctionTemplateBrowser.h"

#include "MantidQtWidgets/Common/UserInputValidator.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"

#include <QFileInfo>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("MSDFit");
std::vector<std::string> MSDFIT_HIDDEN_PROPS = std::vector<std::string>(
    {"CreateOutput", "LogValue", "PassWSIndexToFunction", "ConvolveMembers", "OutputCompositeMembers",
     "OutputWorkspace", "IgnoreInvalidData", "Output", "PeakRadius", "PlotParameter"});

const std::string MSDGAUSSFUNC{"MsdGauss"};
const std::string MSDPETERSFUNC{"MsdPeters"};
const std::string MSDYIFUNC{"MsdYi"};

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

auto msdFunctionStrings =
    std::map<std::string, std::string>({{"None", ""},
                                        {"Gauss", "name=MsdGauss,Height=1,Msd=0.05,constraints=(Height>0, Msd>0)"},
                                        {"Peters", "name=MsdPeters,Height=1,Msd=0.05,Beta=1,constraints=(Height>0, "
                                                   "Msd>0, Beta>0)"},
                                        {"Yi", "name=MsdYi,Height=1,Msd=0.05,Sigma=1,constraints=(Height>0, Msd>0, "
                                               "Sigma>0)"}});

IndirectDataAnalysisMSDFitTab::IndirectDataAnalysisMSDFitTab(QWidget *parent)
    : IndirectFitAnalysisTab(new MSDFitModel, parent), m_uiForm(new Ui::IndirectFitTab) {
  m_uiForm->setupUi(parent);

  m_msdFittingModel = dynamic_cast<MSDFitModel *>(getFittingModel());
  setFitDataPresenter(std::make_unique<IndirectFitDataPresenter>(m_msdFittingModel, m_uiForm->dockArea->m_fitDataView));
  setPlotView(m_uiForm->dockArea->m_fitPlotView);
  setSpectrumSelectionView(m_uiForm->svSpectrumView);
  setOutputOptionsView(m_uiForm->ovOutputOptionsView);
  auto parameterEstimation = createParameterEstimation();
  auto templateBrowser = new SingleFunctionTemplateBrowser(
      msdFunctionStrings, std::make_unique<IDAFunctionParameterEstimation>(parameterEstimation));
  m_uiForm->dockArea->m_fitPropertyBrowser->setFunctionTemplateBrowser(templateBrowser);
  setFitPropertyBrowser(m_uiForm->dockArea->m_fitPropertyBrowser);
  m_uiForm->dockArea->m_fitPropertyBrowser->setHiddenProperties(MSDFIT_HIDDEN_PROPS);

  setEditResultVisible(false);
  respondToFunctionChanged();
  fitFunctionChanged();
}

void IndirectDataAnalysisMSDFitTab::setupFitTab() {
  connect(this, SIGNAL(functionChanged()), this, SLOT(fitFunctionChanged()));
  connect(m_uiForm->pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
}

void IndirectDataAnalysisMSDFitTab::runClicked() { runTab(); }

void IndirectDataAnalysisMSDFitTab::setRunIsRunning(bool running) {
  m_uiForm->pbRun->setText(running ? "Running..." : "Run");
}

void IndirectDataAnalysisMSDFitTab::setRunEnabled(bool enable) { m_uiForm->pbRun->setEnabled(enable); }

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

void IndirectDataAnalysisMSDFitTab::fitFunctionChanged() { m_msdFittingModel->setFitTypeString(getFitTypeString()); }

std::string IndirectDataAnalysisMSDFitTab::getFitTypeString() const {
  // This function attempts to work out which fit type is being done. It will
  // currently only recognise the three default types.
  const auto numberOfGauss = getNumberOfCustomFunctions("MsdGauss");
  const auto numberOfPeters = getNumberOfCustomFunctions("MsdPeters");
  const auto numberOfYi = getNumberOfCustomFunctions("MsdYi");

  if (numberOfGauss + numberOfPeters + numberOfYi != 1) {
    return "UserDefined";
  }

  if (numberOfGauss == 1)
    return "Gauss";

  if (numberOfPeters == 1)
    return "Peters";

  if (numberOfYi == 1)
    return "Yi";

  return "UserDefined";
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
