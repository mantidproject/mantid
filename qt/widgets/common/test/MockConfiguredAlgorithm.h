// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockConfiguredAlgorithm : public MantidQt::API::IConfiguredAlgorithm {
public:
  MOCK_CONST_METHOD0(algorithm, Mantid::API::IAlgorithm_sptr());
  MOCK_CONST_METHOD0(properties, AlgorithmRuntimeProps());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
