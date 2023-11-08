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
#include "IDAFunctionParameterEstimation.h"

#include "FunctionBrowser/SingleFunctionTemplateBrowser.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidKernel/PhysicalConstants.h"

#include <string>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces::IDA;

namespace {
constexpr double HBAR = Mantid::PhysicalConstants::h / Mantid::PhysicalConstants::meV * 1e12 / (2 * M_PI);

std::vector<std::string> FQFIT_HIDDEN_PROPS =
    std::vector<std::string>({"CreateOutput", "LogValue", "PassWSIndexToFunction", "ConvolveMembers",
                              "OutputCompositeMembers", "OutputWorkspace", "Output", "PeakRadius", "PlotParameter"});

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

namespace MantidQt::CustomInterfaces::IDA {

IndirectDataAnalysisFqFitTab::IndirectDataAnalysisFqFitTab(QWidget *parent)
    : IndirectDataAnalysisTab(
          new FqFitModel,
          new SingleFunctionTemplateBrowser(
              widthFits, std::make_unique<IDAFunctionParameterEstimation>(createParameterEstimation())),
          new FqFitDataView, FQFIT_HIDDEN_PROPS, parent) {
  // m_fitPropertyBrowser->updateAvailableFunctions(availableFits.at(DataType::ALL));

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

void IndirectDataAnalysisFqFitTab::addDataToModel(IAddWorkspaceDialog const *dialog) {
  if (const auto fqFitDialog = dynamic_cast<FqFitAddWorkspaceDialog const *>(dialog)) {
    m_dataPresenter->addWorkspace(fqFitDialog->workspaceName(), fqFitDialog->parameterType(),
                                  fqFitDialog->parameterNameIndex());
    m_fittingModel->addDefaultParameters();
    setActiveWorkspaceIDToCurrentWorkspace(fqFitDialog);
    setModelSpectrum(fqFitDialog->parameterNameIndex(), fqFitDialog->parameterType());
    m_activeWorkspaceID = m_dataPresenter->getNumberOfWorkspaces();
  }
}

void IndirectDataAnalysisFqFitTab::setActiveWorkspaceIDToCurrentWorkspace(IAddWorkspaceDialog const *dialog) {
  //  update active data index with correct index based on the workspace name
  //  and the vector in m_fitDataModel which is in the base class
  //  indirectFittingModel get table workspace index
  const auto wsName = dialog->workspaceName().append("_HWHM");
  // This a vector of workspace names currently loaded
  auto wsVector = m_fittingModel->getFitDataModel()->getWorkspaceNames();
  // this is an iterator pointing to the current wsName in wsVector
  auto wsIt = std::find(wsVector.begin(), wsVector.end(), wsName);
  // this is the index of the workspace.
  const auto index = WorkspaceID(std::distance(wsVector.begin(), wsIt));
  m_activeWorkspaceID = index;
}

void IndirectDataAnalysisFqFitTab::setModelSpectrum(int index, const std::string &paramType) {
  if (index < 0)
    throw std::runtime_error("No valid parameter was selected.");
  else if (paramType == "Width")
    m_dataPresenter->setActiveWidth(static_cast<std::size_t>(index), m_activeWorkspaceID, false);
  else
    m_dataPresenter->setActiveEISF(static_cast<std::size_t>(index), m_activeWorkspaceID, false);
}

IDAFunctionParameterEstimation IndirectDataAnalysisFqFitTab::createParameterEstimation() const {

  IDAFunctionParameterEstimation parameterEstimation;
  parameterEstimation.addParameterEstimationFunction("ChudleyElliot", estimateChudleyElliot);
  parameterEstimation.addParameterEstimationFunction("HallRoss", estimateHallRoss);
  parameterEstimation.addParameterEstimationFunction("TeixeiraWater", estimateTeixeiraWater);
  parameterEstimation.addParameterEstimationFunction("FickDiffusion", estimateFickDiffusion);

  return parameterEstimation;
}

} // namespace MantidQt::CustomInterfaces::IDA
