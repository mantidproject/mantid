// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/AlgorithmRunners/IAsyncAlgorithmRunner.h"

#include <gmock/gmock.h>

namespace MantidQt::API {
class IAsyncAlgorithmSubscriber;
}

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockAsyncAlgorithmRunner : public MantidQt::API::IAsyncAlgorithmRunner {

public:
  MOCK_METHOD1(subscribe, void(MantidQt::API::IAsyncAlgorithmSubscriber *subscriber));

  MOCK_METHOD0(cancelRunningAlgorithm, void());

  MOCK_METHOD1(startAlgorithmImpl, void(Mantid::API::IAlgorithm_sptr alg));
  MOCK_CONST_METHOD0(getAlgorithm, Mantid::API::IAlgorithm_sptr());

  // Wrapper around startAlgorithmImpl to allow us to record
  // which algorithm was started
  void startAlgorithm(Mantid::API::IAlgorithm_sptr alg) {
    m_algorithm = alg;
    startAlgorithmImpl(alg);
  }

  Mantid::API::IAlgorithm_sptr algorithm() const { return m_algorithm; }

private:
  Mantid::API::IAlgorithm_sptr m_algorithm;
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
