// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFAnalysisModel.h"

#include "MantidAPI/AlgorithmProperties.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"

#include <algorithm>
#include <numeric>

using namespace Mantid::API;

namespace {

std::string const NOT_IN_ADS = "not_stored_in_ads";
std::string const WS_EXPORT_NAME("ALFView_exported");

IFunction_sptr createFlatBackground(double const height = 0.0) {
  auto flatBackground = FunctionFactory::Instance().createFunction("FlatBackground");
  flatBackground->setParameter("A0", height);
  flatBackground->addConstraints("A0 > 0");
  return flatBackground;
}

IFunction_sptr createGaussian(double const height = 0.0, double const peakCentre = 0.0, double const sigma = 0.0) {
  auto gaussian = FunctionFactory::Instance().createFunction("Gaussian");
  gaussian->setParameter("Height", height);
  gaussian->setParameter("PeakCentre", peakCentre);
  gaussian->setParameter("Sigma", sigma);
  gaussian->addConstraints("Height > 0");
  return gaussian;
}

IFunction_sptr createGaussian(Mantid::MantidVec const &xData, Mantid::MantidVec const &yData,
                              double const backgroundHeight) {
  const auto maxValue = *std::max_element(yData.begin(), yData.end());

  auto sigma(0.0);
  auto centre(0.0);
  auto isMaximum(false);
  for (auto i = 0u; i < yData.size(); ++i) {
    if (yData[i] == maxValue && !isMaximum) {
      isMaximum = true;
      centre = xData[i];
    }
    if (isMaximum && yData[i] < maxValue / 2.0) {
      isMaximum = false;
      sigma = xData[i] - centre;
    }
  }

  return createGaussian(maxValue - backgroundHeight, centre, sigma);
}

CompositeFunction_sptr createCompositeFunction(IFunction_sptr const &flatBackground, IFunction_sptr const &gaussian) {
  auto composite = std::make_shared<CompositeFunction>();
  composite->addFunction(flatBackground);
  composite->addFunction(gaussian);
  return composite;
}

} // namespace

namespace MantidQt::CustomInterfaces {

ALFAnalysisModel::ALFAnalysisModel()
    : m_function(createCompositeFunction(createFlatBackground(), createGaussian())), m_fitStatus(""), m_twoThetas(),
      m_extractedWorkspace(), m_fitWorkspace() {}

void ALFAnalysisModel::clear() {
  m_extractedWorkspace = nullptr;
  m_fitWorkspace = nullptr;
  m_fitStatus = "";
  m_twoThetas.clear();
}

void ALFAnalysisModel::setExtractedWorkspace(Mantid::API::MatrixWorkspace_sptr const &workspace,
                                             std::vector<double> const &twoThetas) {
  m_extractedWorkspace = workspace;
  m_twoThetas = twoThetas;
  m_fitStatus = "";
  m_fitWorkspace = nullptr;
}

Mantid::API::MatrixWorkspace_sptr ALFAnalysisModel::extractedWorkspace() const { return m_extractedWorkspace; }

bool ALFAnalysisModel::isDataExtracted() const { return m_extractedWorkspace != nullptr; }

void ALFAnalysisModel::calculateEstimate(MatrixWorkspace_sptr const &workspace) {
  if (workspace) {
    m_function = calculateEstimateImpl(workspace);
  } else {
    m_function = createCompositeFunction(createFlatBackground(), createGaussian());
  }
  m_fitStatus = "";
  m_fitWorkspace = nullptr;
}

IFunction_sptr ALFAnalysisModel::calculateEstimateImpl(MatrixWorkspace_sptr const &workspace) {
  auto const xData = workspace->readX(0);
  auto const yData = workspace->readY(0);

  auto const backgroundHeight = std::accumulate(yData.begin(), yData.end(), 0.0) / static_cast<double>(yData.size());

  return createCompositeFunction(createFlatBackground(backgroundHeight),
                                 createGaussian(xData, yData, backgroundHeight));
}

void ALFAnalysisModel::exportWorkspaceCopyToADS() const {
  // The ADS should not be used anywhere else apart from here. Note that a copy is exported.
  if (auto const workspace = plottedWorkspace()) {
    AnalysisDataService::Instance().addOrReplace(WS_EXPORT_NAME, workspace->clone());
  }
}

MatrixWorkspace_sptr ALFAnalysisModel::plottedWorkspace() const {
  if (m_fitWorkspace) {
    return m_fitWorkspace;
  }
  if (m_extractedWorkspace) {
    return m_extractedWorkspace;
  }
  return nullptr;
}

std::vector<int> ALFAnalysisModel::plottedWorkspaceIndices() const {
  return m_fitWorkspace ? std::vector<int>{0, 1} : std::vector<int>{0};
}

void ALFAnalysisModel::setPeakParameters(Mantid::API::IPeakFunction_const_sptr const &peak) {
  auto const centre = peak->getParameter("PeakCentre");
  auto const height = peak->getParameter("Height");
  auto const sigma = peak->getParameter("Sigma");

  setPeakCentre(centre);
  m_function->setParameter("f1.Height", height);
  m_function->setParameter("f1.Sigma", sigma);
}

void ALFAnalysisModel::setPeakCentre(double const centre) {
  m_function->setParameter("f1.PeakCentre", centre);
  m_fitStatus = "";
}

double ALFAnalysisModel::peakCentre() const { return m_function->getParameter("f1.PeakCentre"); }

double ALFAnalysisModel::background() const { return m_function->getParameter("f0.A0"); }

Mantid::API::IPeakFunction_const_sptr ALFAnalysisModel::getPeakCopy() const {
  auto const gaussian = m_function->getFunction(1)->clone();
  return std::dynamic_pointer_cast<Mantid::API::IPeakFunction>(gaussian);
}

std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
ALFAnalysisModel::cropWorkspaceProperties(std::pair<double, double> const &range) const {
  auto properties = std::make_unique<AlgorithmRuntimeProps>();
  AlgorithmProperties::update("InputWorkspace", m_extractedWorkspace, *properties);
  AlgorithmProperties::update("XMin", range.first, *properties);
  AlgorithmProperties::update("XMax", range.second, *properties);
  AlgorithmProperties::update("OutputWorkspace", NOT_IN_ADS, *properties);
  return properties;
}

std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
ALFAnalysisModel::fitProperties(std::pair<double, double> const &range) const {
  // Cast to the workspace type accepted by the Fit algorithm. Failure to do this will cause an exception.
  Mantid::API::Workspace_sptr workspace = m_extractedWorkspace;

  auto properties = std::make_unique<AlgorithmRuntimeProps>();
  AlgorithmProperties::update("Function", m_function, *properties);
  AlgorithmProperties::update("InputWorkspace", workspace, *properties);
  AlgorithmProperties::update("CreateOutput", true, *properties);
  AlgorithmProperties::update("StartX", range.first, *properties);
  AlgorithmProperties::update("EndX", range.second, *properties);
  return properties;
}

void ALFAnalysisModel::setFitResult(Mantid::API::MatrixWorkspace_sptr workspace, Mantid::API::IFunction_sptr function,
                                    std::string fitStatus) {
  m_fitWorkspace = std::move(workspace);
  m_function = std::move(function);
  m_fitStatus = std::move(fitStatus);
}

std::string ALFAnalysisModel::fitStatus() const { return m_fitStatus; }

std::size_t ALFAnalysisModel::numberOfTubes() const { return m_twoThetas.size(); }

std::optional<double> ALFAnalysisModel::averageTwoTheta() const {
  if (m_twoThetas.empty()) {
    return std::nullopt;
  }
  return std::reduce(m_twoThetas.cbegin(), m_twoThetas.cend()) / static_cast<double>(numberOfTubes());
}

std::optional<double> ALFAnalysisModel::rotationAngle() const {
  if (m_fitStatus.empty()) {
    return std::nullopt;
  }
  auto const twoTheta = averageTwoTheta();
  if (!twoTheta) {
    return std::nullopt;
  }
  return peakCentre() / (2 * sin((*twoTheta / 2.0) * M_PI / 180.0));
}

} // namespace MantidQt::CustomInterfaces
