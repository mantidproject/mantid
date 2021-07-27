// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataAnalysisFqFitTab.h"
#include "FQFitConstants.h"
#include "FqFitDataPresenter.h"
#include "IDAFunctionParameterEstimation.h"

#include "IndirectFunctionBrowser/SingleFunctionTemplateBrowser.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <boost/algorithm/string/join.hpp>

#include <string>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

namespace {
constexpr double HBAR = Mantid::PhysicalConstants::h / Mantid::PhysicalConstants::meV * 1e12 / (2 * M_PI);
}

std::vector<std::string> FQFIT_HIDDEN_PROPS = std::vector<std::string>(
    {"CreateOutput", "LogValue", "PassWSIndexToFunction", "ConvolveMembers", "OutputCompositeMembers",
     "OutputWorkspace", "IgnoreInvalidData", "Output", "PeakRadius", "PlotParameter"});

IndirectDataAnalysisFqFitTab::IndirectDataAnalysisFqFitTab(QWidget *parent)
    : IndirectFitAnalysisTab(new FqFitModel, parent), m_uiForm(new Ui::IndirectFitTab) {
  m_uiForm->setupUi(parent);

  m_FqFittingModel = dynamic_cast<FqFitModel *>(getFittingModel());
  auto parameterEstimation = createParameterEstimation();
  auto templateBrowser = new SingleFunctionTemplateBrowser(
      widthFits, std::make_unique<IDAFunctionParameterEstimation>(parameterEstimation));
  setPlotView(m_uiForm->dockArea->m_fitPlotView);
  m_plotPresenter->setXBounds({0.0, 2.0});
  setFitDataPresenter(
      std::make_unique<FqFitDataPresenter>(m_FqFittingModel, m_uiForm->dockArea->m_fitDataView, templateBrowser));
  setOutputOptionsView(m_uiForm->ovOutputOptionsView);

  m_uiForm->dockArea->m_fitPropertyBrowser->setFunctionTemplateBrowser(templateBrowser);
  templateBrowser->updateAvailableFunctions(availableFits.at(DataType::ALL));
  setFitPropertyBrowser(m_uiForm->dockArea->m_fitPropertyBrowser);
  m_uiForm->dockArea->m_fitPropertyBrowser->setHiddenProperties(FQFIT_HIDDEN_PROPS);

  setEditResultVisible(false);
}

void IndirectDataAnalysisFqFitTab::setupFitTab() {
  connect(m_uiForm->pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(this, SIGNAL(functionChanged()), this, SLOT(updateModelFitTypeString()));
}

void IndirectDataAnalysisFqFitTab::updateModelFitTypeString() {
  m_FqFittingModel->setFitTypeString(getFitTypeString());
}

std::string IndirectDataAnalysisFqFitTab::getFitTypeString() const {
  if (!m_FqFittingModel->getFitFunction() || m_FqFittingModel->getFitFunction()->nFunctions() == 0) {
    return "NoCurrentFunction";
  }

  auto fun = m_FqFittingModel->getFitFunction()->getFunction(0);
  if (fun->nFunctions() == 0) {
    return fun->name();
  } else {
    return "UserDefinedCompositeFunction";
  }
}

void IndirectDataAnalysisFqFitTab::runClicked() { runTab(); }

void IndirectDataAnalysisFqFitTab::setRunIsRunning(bool running) {
  m_uiForm->pbRun->setText(running ? "Running..." : "Run");
}

void IndirectDataAnalysisFqFitTab::setRunEnabled(bool enable) { m_uiForm->pbRun->setEnabled(enable); }

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

namespace {
void estimateChudleyElliot(::Mantid::API::IFunction_sptr &function, const DataForParameterEstimation &estimationData) {

  auto y = estimationData.y;
  auto x = estimationData.x;
  if (x.size() != 2 || y.size() != 2) {
    return;
  }

  double L = 1.5;
  double tau = (HBAR / y[1]) * (1 - sin(x[1] * L) / (L * x[1]));

  function->setParameter("L", L);
  function->setParameter("Tau", tau);
}
void estimateHallRoss(::Mantid::API::IFunction_sptr &function, const DataForParameterEstimation &estimationData) {

  auto y = estimationData.y;
  auto x = estimationData.x;
  if (x.size() != 2 || y.size() != 2) {
    return;
  }

  double L = 0.2;
  double tau = (HBAR / y[1]) * (1 - exp((-x[1] * x[1] * L * L) / 2));

  function->setParameter("L", L);
  function->setParameter("Tau", tau);
}
void estimateTeixeiraWater(::Mantid::API::IFunction_sptr &function, const DataForParameterEstimation &estimationData) {

  auto y = estimationData.y;
  auto x = estimationData.x;
  if (x.size() != 2 || y.size() != 2) {
    return;
  }

  double L = 1.5;
  double QL = x[1] * L;
  double tau = (HBAR / y[1]) * ((QL * QL) / (6 + QL * QL));

  function->setParameter("L", L);
  function->setParameter("Tau", tau);
}
void estimateFickDiffusion(::Mantid::API::IFunction_sptr &function, const DataForParameterEstimation &estimationData) {
  auto y = estimationData.y;
  auto x = estimationData.x;
  if (x.size() != 2 || y.size() != 2) {
    return;
  }

  function->setParameter("D", y[1] / (x[1] * x[1]));
}
} // namespace

IDAFunctionParameterEstimation IndirectDataAnalysisFqFitTab::createParameterEstimation() const {

  IDAFunctionParameterEstimation parameterEstimation;
  parameterEstimation.addParameterEstimationFunction("ChudleyElliot", estimateChudleyElliot);
  parameterEstimation.addParameterEstimationFunction("HallRoss", estimateHallRoss);
  parameterEstimation.addParameterEstimationFunction("TeixeiraWater", estimateTeixeiraWater);
  parameterEstimation.addParameterEstimationFunction("FickDiffusion", estimateFickDiffusion);

  return parameterEstimation;
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
