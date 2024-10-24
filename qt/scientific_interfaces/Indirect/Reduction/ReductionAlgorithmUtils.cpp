// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ReductionAlgorithmUtils.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperties.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidQtWidgets/Common/ConfiguredAlgorithm.h"

namespace MantidQt::CustomInterfaces {

using namespace Mantid::API;

MantidQt::API::IConfiguredAlgorithm_sptr configureAlgorithm(std::string const &algorithmName,
                                                            std::unique_ptr<IAlgorithmRuntimeProps> properties,
                                                            bool const validatePropsPreExec = true) {
  return std::make_unique<MantidQt::API::ConfiguredAlgorithm>(AlgorithmManager::Instance().create(algorithmName),
                                                              std::move(properties), validatePropsPreExec);
}

MantidQt::API::IConfiguredAlgorithm_sptr loadConfiguredAlg(std::string const &filename, std::string const &instrument,
                                                           std::vector<int> const &detectorList,
                                                           std::string const &outputWorkspace) {
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  AlgorithmProperties::update("Filename", filename, *properties);
  AlgorithmProperties::update("OutputWorkspace", outputWorkspace, *properties);
  if (instrument == "TFXA") {
    AlgorithmProperties::update("LoadLogFiles", false, *properties);
    AlgorithmProperties::update("SpectrumMin", detectorList.front(), *properties);
    AlgorithmProperties::update("SpectrumMax", detectorList.back(), *properties);
  }
  return configureAlgorithm("Load", std::move(properties), false);
}

MantidQt::API::IConfiguredAlgorithm_sptr calculateFlatBackgroundConfiguredAlg(std::string const &inputWorkspace,
                                                                              double const startX, double const endX,
                                                                              std::string const &outputWorkspace) {
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  AlgorithmProperties::update("InputWorkspace", inputWorkspace, *properties);
  AlgorithmProperties::update("Mode", std::string("Mean"), *properties);
  AlgorithmProperties::update("StartX", startX, *properties);
  AlgorithmProperties::update("EndX", endX, *properties);
  AlgorithmProperties::update("OutputWorkspace", outputWorkspace, *properties);
  return configureAlgorithm("CalculateFlatBackground", std::move(properties));
}

MantidQt::API::IConfiguredAlgorithm_sptr groupDetectorsConfiguredAlg(std::string const &inputWorkspace,
                                                                     std::vector<int> const &detectorList,
                                                                     std::string const &outputWorkspace) {
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  AlgorithmProperties::update("InputWorkspace", inputWorkspace, *properties);
  AlgorithmProperties::update("DetectorList", detectorList, *properties, false);
  AlgorithmProperties::update("OutputWorkspace", outputWorkspace, *properties);
  return configureAlgorithm("GroupDetectors", std::move(properties));
}

} // namespace MantidQt::CustomInterfaces
