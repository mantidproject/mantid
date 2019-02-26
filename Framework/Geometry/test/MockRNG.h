// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_MOCKRNG_H_
#define MANTID_GEOMETRY_MOCKRNG_H_

#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

// -----------------------------------------------------------------------------
// Mock Random Number Generator
// -----------------------------------------------------------------------------
class MockRNG : public Mantid::Kernel::PseudoRandomNumberGenerator {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD0(nextValue, double());
  MOCK_METHOD2(nextValue, double(double, double));
  MOCK_METHOD2(nextInt, int(int, int));
  MOCK_METHOD0(restart, void());
  MOCK_METHOD0(save, void());
  MOCK_METHOD0(restore, void());
  MOCK_METHOD1(setSeed, void(size_t));
  MOCK_METHOD2(setRange, void(const double, const double));
  MOCK_CONST_METHOD0(min, double());
  MOCK_CONST_METHOD0(max, double());
  GNU_DIAG_ON_SUGGEST_OVERRIDE
};

#endif /* MANTID_GEOMETRY_MOCKRNG_H_ */
