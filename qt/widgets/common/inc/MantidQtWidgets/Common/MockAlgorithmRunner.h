// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_MANTIDWIDGETS_MOCKALGORITHMRUNNER_H
#define MANTIDQT_MANTIDWIDGETS_MOCKALGORITHMRUNNER_H

#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"

#include <gmock/gmock.h>

using namespace MantidQt::API;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockAlgorithmRunner : public AlgorithmRunner {
public:
  MockAlgorithmRunner() = default;
  MOCK_METHOD0(cancelRunningAlgorithm, void());
  MOCK_METHOD1(startAlgorithm, void(Mantid::API::IAlgorithm_sptr));
  MOCK_CONST_METHOD0(getAlgorithm, Mantid::API::IAlgorithm_sptr());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

#endif
