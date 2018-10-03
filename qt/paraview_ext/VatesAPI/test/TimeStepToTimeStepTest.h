// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef TIMESTEP_TO_TIMESTEP_TEST_H_
#define TIMESTEP_TO_TIMESTEP_TEST_H_

#include "MantidVatesAPI/TimeStepToTimeStep.h"
#include <cxxtest/TestSuite.h>

class TimeStepToTimeStepTest : public CxxTest::TestSuite {
public:
  void testArgumentEqualsProduct() {
    // Check that this type works as a compile-time proxy. Should do nothing
    // with argument other than return it.
    Mantid::VATES::TimeStepToTimeStep proxy;
    int argument = 1;
    TSM_ASSERT_EQUALS(
        "The TimeStepToTimeStep proxy should return its own argument", argument,
        proxy(argument));
  }
};

#endif
