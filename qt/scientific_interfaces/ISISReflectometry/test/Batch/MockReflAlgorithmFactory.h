// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Batch/IReflAlgorithmFactory.h"
#include "Reduction/PreviewRow.h"

#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"

#include <gmock/gmock.h>

class MockReflAlgorithmFactory : public IReflAlgorithmFactory {
public:
  MOCK_METHOD(MantidQt::API::IConfiguredAlgorithm_sptr, makePreprocessingAlgorithm, (PreviewRow &), (const, override));
  MOCK_METHOD(MantidQt::API::IConfiguredAlgorithm_sptr, makeSumBanksAlgorithm, (PreviewRow &), (const, override));
};