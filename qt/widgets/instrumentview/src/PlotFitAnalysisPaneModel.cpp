// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneModel.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"

#include <algorithm>
#include <numeric>

namespace {

MatrixWorkspace_sptr cropWorkspace(const MatrixWorkspace_sptr &workspace, double startX, double endX) {
  auto cropper = AlgorithmManager::Instance().create("CropWorkspace");
  cropper->setAlwaysStoreInADS(false);
  cropper->setProperty("InputWorkspace", workspace);
  cropper->setProperty("OutputWorkspace", "__cropped");
  cropper->setProperty("XMin", startX);
  cropper->setProperty("XMax", endX);
  cropper->execute();
  return cropper->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr convertToPointData(const MatrixWorkspace_sptr &workspace) {
  auto converter = AlgorithmManager::Instance().create("ConvertToPointData");
  converter->setAlwaysStoreInADS(false);
  converter->setProperty("InputWorkspace", workspace);
  converter->setProperty("OutputWorkspace", "__pointData");
  converter->execute();
  return converter->getProperty("OutputWorkspace");
}

IFunction_sptr createFlatBackground(double height = 0.0) {
  auto flatBackground = FunctionFactory::Instance().createFunction("FlatBackground");
  flatBackground->setParameter("A0", height);
  return flatBackground;
}

IFunction_sptr createGaussian(double height = 0.0, double peakCentre = 0.0, double sigma = 0.0) {
  auto gaussian = FunctionFactory::Instance().createFunction("Gaussian");
  gaussian->setParameter("Height", height);
  gaussian->setParameter("PeakCentre", peakCentre);
  gaussian->setParameter("Sigma", sigma);
  return gaussian;
}

IFunction_sptr createGaussian(const Mantid::MantidVec &xData, const Mantid::MantidVec &yData, double backgroundHeight) {
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

CompositeFunction_sptr createCompositeFunction(const IFunction_sptr &flatBackground, const IFunction_sptr &gaussian) {
  auto composite = std::make_shared<CompositeFunction>();
  composite->addFunction(flatBackground);
  composite->addFunction(gaussian);
  return composite;
}

} // namespace

using namespace Mantid::API;

namespace MantidQt::MantidWidgets {

PlotFitAnalysisPaneModel::PlotFitAnalysisPaneModel() : m_estimateFunction(nullptr) {}

IFunction_sptr PlotFitAnalysisPaneModel::doFit(const std::string &wsName, const std::pair<double, double> &range,
                                               const IFunction_sptr func) {

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("Fit");
  alg->initialize();
  alg->setProperty("Function", func);
  alg->setProperty("InputWorkspace", wsName);
  alg->setProperty("Output", wsName + "_fits");
  alg->setProperty("StartX", range.first);
  alg->setProperty("EndX", range.second);
  alg->execute();
  return alg->getProperty("Function");
}

IFunction_sptr PlotFitAnalysisPaneModel::calculateEstimate(const std::string &workspaceName,
                                                           const std::pair<double, double> &range) {
  auto &ads = AnalysisDataService::Instance();
  if (ads.doesExist(workspaceName)) {
    auto workspace = ads.retrieveWS<MatrixWorkspace>(workspaceName);

    m_estimateFunction = calculateEstimate(workspace, range);
    return m_estimateFunction;
  } else {
    m_estimateFunction = nullptr;
    return createCompositeFunction(createFlatBackground(), createGaussian());
  }
}

IFunction_sptr PlotFitAnalysisPaneModel::calculateEstimate(MatrixWorkspace_sptr &workspace,
                                                           const std::pair<double, double> &range) {
  workspace = cropWorkspace(workspace, range.first, range.second);
  workspace = convertToPointData(workspace);

  const auto xData = workspace->readX(0);
  const auto yData = workspace->readY(0);

  const auto background = std::accumulate(yData.begin(), yData.end(), 0.0) / static_cast<double>(yData.size());

  return createCompositeFunction(createFlatBackground(background), createGaussian(xData, yData, background));
}

bool PlotFitAnalysisPaneModel::hasEstimate() const { return m_estimateFunction != nullptr; }

} // namespace MantidQt::MantidWidgets
