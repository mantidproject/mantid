// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "StretchModel.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperties.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"

using namespace Mantid::API;

namespace MantidQt::CustomInterfaces {

MantidQt::API::IConfiguredAlgorithm_sptr StretchModel::stretchAlgorithm(const StretchRunData &algParams,
                                                                        const std::string &fitWorkspaceName,
                                                                        const std::string &contourWorkspaceName,
                                                                        const bool useQuickBayes) const {
  auto properties = std::make_unique<AlgorithmRuntimeProps>();

  properties->setProperty("SampleWorkspace", algParams.sampleName);
  properties->setProperty("ResolutionWorkspace", algParams.resolutionName);
  properties->setProperty("EMin", algParams.eMin);
  properties->setProperty("EMax", algParams.eMax);
  properties->setProperty("NumberBeta", algParams.beta);
  properties->setProperty("Elastic", algParams.elasticPeak);
  properties->setProperty("OutputWorkspaceFit", fitWorkspaceName);
  properties->setProperty("OutputWorkspaceContour", contourWorkspaceName);
  properties->setProperty("Background", algParams.backgroundName);
  if (!useQuickBayes) {
    properties->setProperty("SampleBins", algParams.sampleBinning);
    properties->setProperty("NumberSigma", algParams.sigma);
    properties->setProperty("Loop", algParams.sequentialFit);
  }

  std::string const algorithmName = useQuickBayes ? "BayesStretch2" : "BayesStretch";
  auto stretch = AlgorithmManager::Instance().create(algorithmName);
  stretch->initialize();

  return std::make_shared<API::ConfiguredAlgorithm>(stretch, std::move(properties));
}

API::IConfiguredAlgorithm_sptr StretchModel::setupSaveAlgorithm(const std::string &wsName) const {
  auto saveAlgo = AlgorithmManager::Instance().create("SaveNexusProcessed");
  saveAlgo->initialize();

  auto saveProps = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();

  saveProps->setPropertyValue("Filename", wsName + ".nxs");
  saveProps->setPropertyValue("InputWorkspace", wsName);

  return std::make_shared<API::ConfiguredAlgorithm>(saveAlgo, std::move(saveProps));
}

} // namespace MantidQt::CustomInterfaces
