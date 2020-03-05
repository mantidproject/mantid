// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_MOCKSLITCALCULATOR_H
#define MANTID_MANTIDWIDGETS_MOCKSLITCALCULATOR_H

#include "ISlitCalculator.h"
#include <gmock/gmock.h>
#include <string>

class MockSlitCalculator : public MantidQt::MantidWidgets::ISlitCalculator {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD1(setCurrentInstrumentName, void(std::string));
  MOCK_METHOD0(processInstrumentHasBeenChanged, void());
  MOCK_METHOD0(show, void());
  GNU_DIAG_ON_SUGGEST_OVERRIDE
};

#endif /* MANTID_MANTIDWIDGETS_MOCKSLITCALCULATOR_H */
