// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/AsymmetricPearsonVII.h"

using Mantid::CurveFitting::Functions::AsymmetricPearsonVII;

class AsymmetricPearsonVIITest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AsymmetricPearsonVIITest *createSuite() { return new AsymmetricPearsonVIITest(); }
  static void destroySuite(AsymmetricPearsonVIITest *suite) { delete suite; }

  void test_Something() { TS_ASSERT_DELTA(1.0, 1.0, 1e-10); }
};
