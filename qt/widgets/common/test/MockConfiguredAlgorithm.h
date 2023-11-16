// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IAlgorithmRuntimeProps.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"

#include <gmock/gmock.h>

#include <memory>

class MockConfiguredAlgorithm : public MantidQt::API::IConfiguredAlgorithm {
public:
  MockConfiguredAlgorithm(std::unique_ptr<Mantid::API::IAlgorithmRuntimeProps> runtimeProps)
      : m_runtimeProps(std::move(runtimeProps)) {
    // Explicitly hold a reference to the props we return by reference, so the tests don't get this wrong
    ON_CALL(*this, getAlgorithmRuntimeProps).WillByDefault(::testing::ReturnRef(*m_runtimeProps));
  }

  MOCK_METHOD(Mantid::API::IAlgorithm_sptr, algorithm, (), (const, override));
  MOCK_METHOD((const Mantid::API::IAlgorithmRuntimeProps &), getAlgorithmRuntimeProps, (), (const, noexcept, override));
  MOCK_METHOD((bool), validatePropsPreExec, (), (const, override, noexcept));

private:
  std::unique_ptr<Mantid::API::IAlgorithmRuntimeProps> m_runtimeProps;
};
