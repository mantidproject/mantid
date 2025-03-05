// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <sstream>

#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument/Parameter.h"
#include "MantidGeometry/Instrument/ParameterFactory.h"

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class FitParameterTest : public CxxTest::TestSuite {
public:
  void test1() {
    FitParameter fitP;

    fitP.setValue(9.1);
    fitP.setTie("bob");

    TS_ASSERT_DELTA(fitP.getValue(), 9.1, 0.0001);
    TS_ASSERT(fitP.getTie().compare("bob") == 0);
  }

  void testReadInOut() {
    FitParameter fitP1;

    std::istringstream input("9.1 , fido , , , , 8.2 , tie , formula, TOF , dSpacing,");
    input >> fitP1;
    std::stringstream inout;
    inout << fitP1;

    FitParameter fitP;
    inout >> fitP;

    TS_ASSERT_DELTA(fitP.getValue(), 9.1, 0.0001);
    TS_ASSERT(fitP.getFunction().compare("fido") == 0);
    TS_ASSERT(fitP.getName().compare("") == 0);
    TS_ASSERT(fitP.getConstraintPenaltyFactor().compare("8.2") == 0);
    TS_ASSERT(fitP.getTie().compare("tie") == 0);
    TS_ASSERT(fitP.getFormula().compare("formula") == 0);
    TS_ASSERT(fitP.getFormulaUnit().compare("TOF") == 0);
    TS_ASSERT(fitP.getResultUnit().compare("dSpacing") == 0);
  }

  void test2() {
    FitParameter fitP;

    std::istringstream input("9.1 , fido , , , , , tie , formula, TOF, dSpacing ,");

    input >> fitP;

    TS_ASSERT_DELTA(fitP.getValue(), 9.1, 0.0001);
    TS_ASSERT(fitP.getFunction().compare("fido") == 0);
    TS_ASSERT(fitP.getTie().compare("tie") == 0);
    TS_ASSERT(fitP.getFormula().compare("formula") == 0);
    TS_ASSERT(fitP.getFormulaUnit().compare("TOF") == 0);
  }

  void test3() {
    FitParameter fitP;

    std::istringstream input("9.1 , , , , , , , , ");

    input >> fitP;

    TS_ASSERT_DELTA(fitP.getValue(), 9.1, 0.0001);
    TS_ASSERT(fitP.getTie().compare("") == 0);
  }

  void test4() {
    FitParameter fitP;

    std::istringstream input("bob , , , , , , ,  ");

    input >> fitP;

    TS_ASSERT_DELTA(fitP.getValue(), 0.0, 0.0001);
    TS_ASSERT(fitP.getTie().compare("") == 0);
  }

  void test5() {
    FitParameter fitP;

    std::istringstream input("9.1 , , , , , , , ");

    input >> fitP;

    TS_ASSERT_DELTA(fitP.getValue(), 9.1, 0.0001);
    TS_ASSERT(fitP.getTie().compare("") == 0);
  }

  void test6() {
    FitParameter fitP;

    std::istringstream input("9.1 , , ,  , , , ,   ");

    input >> fitP;

    TS_ASSERT_DELTA(fitP.getValue(), 9.1, 0.0001);
    TS_ASSERT(fitP.getTie().compare("") == 0);
  }

  void test7() {
    FitParameter fitP;

    std::istringstream input("9.1 , function , name , 3, 8 , , , , ,    ");

    input >> fitP;

    TS_ASSERT_DELTA(fitP.getValue(), 9.1, 0.0001);
    TS_ASSERT(fitP.getName().compare("name") == 0);
    TS_ASSERT(fitP.getTie().compare("") == 0);
    TS_ASSERT(fitP.getConstraint().compare("3 < name < 8") == 0);
  }

  void testConstraintMinEmptyandMaxNotEmpty() {
    FitParameter fitP;

    std::istringstream input("9.1 , function , name ,3 ,  , , , , ,    ");

    input >> fitP;

    auto foo = fitP.getConstraint();

    TS_ASSERT(fitP.getConstraint().compare("3 < name") == 0);
  }

  void testConstraintMinNotEmptyandMaxEmpty() {
    FitParameter fitP;

    std::istringstream input("9.1 , function , name , ,8 , , , , ,    ");

    input >> fitP;

    TS_ASSERT(fitP.getConstraint().compare("name < 8") == 0);
  }
};
