// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/IJobRunner.h"

#include <gmock/gmock.h>
#include <memory>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockJobRunner : public MantidQt::API::IJobRunner {
public:
  MOCK_METHOD1(subscribe, void(MantidQt::API::JobRunnerSubscriber *));
  MOCK_METHOD0(clearAlgorithmQueue, void());
  MOCK_METHOD1(setAlgorithmQueue, void(std::deque<MantidQt::API::IConfiguredAlgorithm_sptr>));
  MOCK_METHOD1(executeAlgorithm, void(MantidQt::API::IConfiguredAlgorithm_sptr algorithm));
  MOCK_METHOD0(executeAlgorithmQueue, void());
  MOCK_METHOD0(cancelAlgorithmQueue, void());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
