// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALCPeakFittingModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include <utility>

using namespace Mantid::API;

namespace {

MatrixWorkspace_sptr extractSpectrum(const MatrixWorkspace_sptr &inputWorkspace, const int workspaceIndex) {
  auto extracter = AlgorithmManager::Instance().create("ExtractSingleSpectrum");
  extracter->setAlwaysStoreInADS(false);
  extracter->setProperty("InputWorkspace", inputWorkspace);
  extracter->setProperty("WorkspaceIndex", workspaceIndex);
  extracter->setPropertyValue("OutputWorkspace", "__NotUsed__");
  extracter->execute();
  MatrixWorkspace_sptr output = extracter->getProperty("OutputWorkspace");
  return output;
}

MatrixWorkspace_sptr evaluateFunction(const IFunction_const_sptr &function,
                                      const MatrixWorkspace_sptr &inputWorkspace) {
  auto fit = AlgorithmManager::Instance().create("Fit");
  fit->setAlwaysStoreInADS(false);
  fit->setProperty("Function", function->asString());
  fit->setProperty("InputWorkspace", inputWorkspace);
  fit->setProperty("MaxIterations", 0);
  fit->setProperty("CreateOutput", true);
  fit->execute();
  MatrixWorkspace_sptr output = fit->getProperty("OutputWorkspace");
  return output;
}

} // namespace

namespace MantidQt::CustomInterfaces {

ALCPeakFittingModel::ALCPeakFittingModel(std::unique_ptr<MantidQt::API::IAlgorithmRunner> algorithmRunner)
    : m_algorithmRunner(std::move(algorithmRunner)) {
  m_algorithmRunner->subscribe(this);
}

void ALCPeakFittingModel::subscribe(IALCPeakFittingModelSubscriber *subscriber) { m_subscriber = subscriber; }

void ALCPeakFittingModel::setData(MatrixWorkspace_sptr newData) {
  m_data = std::move(newData);
  m_subscriber->dataChanged();
}

MatrixWorkspace_sptr ALCPeakFittingModel::exportWorkspace() {
  if (m_data) {
    return std::const_pointer_cast<MatrixWorkspace>(m_data);
  }
  return nullptr;
}

ITableWorkspace_sptr ALCPeakFittingModel::exportFittedPeaks() {
  if (m_parameterTable) {

    return m_parameterTable;

  } else {

    return ITableWorkspace_sptr();
  }
}

void ALCPeakFittingModel::setFittedPeaks(IFunction_const_sptr fittedPeaks) {
  m_fittedPeaks = std::move(fittedPeaks);
  m_subscriber->fittedPeaksChanged();
}

void ALCPeakFittingModel::fitPeaks(IFunction_const_sptr peaks) {
  IAlgorithm_sptr fit = AlgorithmManager::Instance().create("Fit");
  fit->setAlwaysStoreInADS(false);
  fit->setRethrows(true);
  fit->setProperty("Function", peaks->asString());
  fit->setProperty("InputWorkspace", std::const_pointer_cast<MatrixWorkspace>(m_data));
  fit->setProperty("CreateOutput", true);
  fit->setProperty("OutputCompositeMembers", true);
  auto runtimeProps = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  MantidQt::API::IConfiguredAlgorithm_sptr fitAlg =
      std::make_shared<MantidQt::API::ConfiguredAlgorithm>(fit, std::move(runtimeProps));

  m_algorithmRunner->execute(std::move(fitAlg));
}

void ALCPeakFittingModel::notifyBatchComplete(MantidQt::API::IConfiguredAlgorithm_sptr &confAlgorithm,
                                              bool /*unused*/) {
  auto const &alg = confAlgorithm->algorithm();
  m_data = alg->getProperty("OutputWorkspace");
  m_parameterTable = alg->getProperty("OutputParameters");
  setFittedPeaks(static_cast<IFunction_sptr>(alg->getProperty("Function")));
}

void ALCPeakFittingModel::notifyAlgorithmError(MantidQt::API::IConfiguredAlgorithm_sptr &algorithm,
                                               const std::string &message) {
  std::string msg = algorithm->algorithm()->name() + " Algorithm failed.\n\n" + std::string(message) + "\n";
  m_subscriber->errorInModel(msg);
}

MatrixWorkspace_sptr ALCPeakFittingModel::guessData(IFunction_const_sptr function, const std::vector<double> &xValues) {
  const auto inputWorkspace = std::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create("Workspace2D", 1, xValues.size(), xValues.size()));
  inputWorkspace->mutableX(0) = xValues;
  return extractSpectrum(evaluateFunction(function, inputWorkspace), 1);
}

} // namespace MantidQt::CustomInterfaces
