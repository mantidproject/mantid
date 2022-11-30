// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFAnalysisModel.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <algorithm>
#include <numeric>

using namespace Mantid::API;

namespace {

MatrixWorkspace_sptr cropWorkspace(MatrixWorkspace_sptr const &workspace, double const startX, double const endX) {
  auto cropper = AlgorithmManager::Instance().create("CropWorkspace");
  cropper->setAlwaysStoreInADS(false);
  cropper->setProperty("InputWorkspace", workspace);
  cropper->setProperty("OutputWorkspace", "__cropped");
  cropper->setProperty("XMin", startX);
  cropper->setProperty("XMax", endX);
  cropper->execute();
  return cropper->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr convertToPointData(MatrixWorkspace_sptr const &workspace) {
  auto converter = AlgorithmManager::Instance().create("ConvertToPointData");
  converter->setAlwaysStoreInADS(false);
  converter->setProperty("InputWorkspace", workspace);
  converter->setProperty("OutputWorkspace", "__pointData");
  converter->execute();
  return converter->getProperty("OutputWorkspace");
}

IFunction_sptr createFlatBackground(double const height = 0.0) {
  auto flatBackground = FunctionFactory::Instance().createFunction("FlatBackground");
  flatBackground->setParameter("A0", height);
  return flatBackground;
}

IFunction_sptr createGaussian(double const height = 0.0, double const peakCentre = 0.0, double const sigma = 0.0) {
  auto gaussian = FunctionFactory::Instance().createFunction("Gaussian");
  gaussian->setParameter("Height", height);
  gaussian->setParameter("PeakCentre", peakCentre);
  gaussian->setParameter("Sigma", sigma);
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
      m_extractedWorkspace() {}

void ALFAnalysisModel::clear() {
  m_extractedWorkspace = nullptr;
  m_fitStatus = "";
  m_twoThetas.clear();
}

void ALFAnalysisModel::setExtractedWorkspace(Mantid::API::MatrixWorkspace_sptr const &workspace,
                                             std::vector<double> const &twoThetas) {
  m_extractedWorkspace = workspace;
  m_twoThetas = twoThetas;
  m_fitStatus = "";
}

Mantid::API::MatrixWorkspace_sptr ALFAnalysisModel::extractedWorkspace() const { return m_extractedWorkspace; }

bool ALFAnalysisModel::isDataExtracted() const { return m_extractedWorkspace != nullptr; }

MatrixWorkspace_sptr ALFAnalysisModel::doFit(std::pair<double, double> const &range) {

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("Fit");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("Function", m_function);
  alg->setProperty("InputWorkspace", m_extractedWorkspace);
  alg->setProperty("CreateOutput", true);
  alg->setProperty("StartX", range.first);
  alg->setProperty("EndX", range.second);
  alg->execute();

  m_function = alg->getProperty("Function");
  m_fitStatus = alg->getPropertyValue("OutputStatus");

  return alg->getProperty("OutputWorkspace");
}

void ALFAnalysisModel::calculateEstimate(std::pair<double, double> const &range) {
  if (m_extractedWorkspace) {
    m_function = calculateEstimate(m_extractedWorkspace, range);
  } else {
    m_function = createCompositeFunction(createFlatBackground(), createGaussian());
  }
  m_fitStatus = "";
}

IFunction_sptr ALFAnalysisModel::calculateEstimate(MatrixWorkspace_sptr &workspace,
                                                   std::pair<double, double> const &range) {
  if (auto alteredWorkspace = cropWorkspace(workspace, range.first, range.second)) {
    alteredWorkspace = convertToPointData(alteredWorkspace);

    auto const xData = alteredWorkspace->readX(0);
    auto const yData = alteredWorkspace->readY(0);

    auto const background = std::accumulate(yData.begin(), yData.end(), 0.0) / static_cast<double>(yData.size());

    return createCompositeFunction(createFlatBackground(background), createGaussian(xData, yData, background));
  }
  return createCompositeFunction(createFlatBackground(), createGaussian());
}

void ALFAnalysisModel::setPeakCentre(double const centre) {
  m_function->setParameter("f1.PeakCentre", centre);
  m_fitStatus = "";
}

double ALFAnalysisModel::peakCentre() const { return m_function->getParameter("f1.PeakCentre"); }

std::string ALFAnalysisModel::fitStatus() const { return m_fitStatus; }

std::size_t ALFAnalysisModel::numberOfTubes() const { return m_twoThetas.size(); }

std::optional<double> ALFAnalysisModel::averageTwoTheta() const {
  if (m_twoThetas.empty()) {
    return std::nullopt;
  }
  return std::reduce(m_twoThetas.cbegin(), m_twoThetas.cend()) / static_cast<double>(numberOfTubes());
}

} // namespace MantidQt::CustomInterfaces
