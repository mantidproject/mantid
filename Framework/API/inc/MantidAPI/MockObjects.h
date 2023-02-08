// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/IAsyncAlgorithmRunner.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

namespace Mantid::API {
class IAsyncAlgorithmSubscriber;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockAsyncAlgorithmRunner : public IAsyncAlgorithmRunner {

public:
  MOCK_METHOD1(subscribe, void(IAsyncAlgorithmSubscriber *subscriber));

  MOCK_METHOD0(cancelRunningAlgorithm, void());

  MOCK_METHOD1(startAlgorithmImpl, void(IAlgorithm_sptr alg));
  MOCK_CONST_METHOD0(getAlgorithm, IAlgorithm_sptr());

  // Wrapper around startAlgorithmImpl to allow us to record
  // which algorithm was started
  void startAlgorithm(IAlgorithm_sptr alg) {
    m_algorithm = alg;
    startAlgorithmImpl(alg);
  }

  IAlgorithm_sptr algorithm() const { return m_algorithm; }

private:
  IAlgorithm_sptr m_algorithm;
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

} // namespace Mantid::API