// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "ALFAlgorithmManager.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidKernel/WarningSuppressions.h"

namespace MantidQt {
namespace CustomInterfaces {

class IALFAlgorithmManagerSubscriber;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockALFAlgorithmManager : public IALFAlgorithmManager {
public:
  MOCK_METHOD1(subscribe, void(IALFAlgorithmManagerSubscriber *subscriber));

  // The algorithms used to load and normalise the Sample
  MOCK_METHOD1(load, void(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties));
  MOCK_METHOD1(normaliseByCurrent, void(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties));
  MOCK_METHOD1(rebinToWorkspace, void(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties));
  MOCK_METHOD1(divide, void(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties));
  MOCK_METHOD1(replaceSpecialValues, void(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties));
  MOCK_METHOD1(convertUnits, void(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties));

  // The algorithms used to produce an Out of plane angle workspace
  MOCK_METHOD1(createWorkspace, void(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties));
  MOCK_METHOD1(scaleX, void(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties));
  MOCK_METHOD1(rebunch, void(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties));

  // The algorithms used for fitting the extracted Out of plane angle workspace
  MOCK_METHOD1(cropWorkspace, void(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties));
  MOCK_METHOD1(fit, void(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

} // namespace CustomInterfaces
} // namespace MantidQt
