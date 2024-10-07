// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/QtAlgorithmRunner.h"

#include <gmock/gmock.h>

using namespace MantidQt::API;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockQtAlgorithmRunner : public QtAlgorithmRunner {
public:
  MockQtAlgorithmRunner() = default;
  MOCK_METHOD1(startAlgorithmImpl, void(Mantid::API::IAlgorithm_sptr));
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
