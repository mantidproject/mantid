// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/IAlgorithmRunnerSubscriber.h"

#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockAlgorithmRunner : public MantidQt::API::IAlgorithmRunner {
public:
  virtual ~MockAlgorithmRunner() = default;

  MOCK_METHOD1(subscribe, void(MantidQt::API::IAlgorithmRunnerSubscriber *subscriber));

  MOCK_METHOD1(execute, void(MantidQt::API::IConfiguredAlgorithm_sptr algorithm));
  MOCK_METHOD1(execute, void(std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> algorithmQueue));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
