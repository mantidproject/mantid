// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ISlitCalculator.h"
#include <gmock/gmock.h>
#include <string>

class MockSlitCalculator : public MantidQt::MantidWidgets::ISlitCalculator {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD1(setCurrentInstrumentName, void(const std::string &));
  MOCK_METHOD0(processInstrumentHasBeenChanged, void());
  MOCK_METHOD0(show, void());
  GNU_DIAG_ON_SUGGEST_OVERRIDE
};
