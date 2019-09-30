// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SPECTRAAXISTEST_H_
#define SPECTRAAXISTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/SpectraAxis.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

// Now the unit test class itself
class SpectraAxisTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpectraAxisTest *createSuite() { return new SpectraAxisTest(); }
  static void destroySuite(SpectraAxisTest *suite) { delete suite; }

  SpectraAxisTest() {
    ws = new WorkspaceTester;
    ws->initialize(5, 1, 1);
    spectraAxis = new SpectraAxis(ws);
    spectraAxis->title() = "A spectra axis";
  }

  ~SpectraAxisTest() override {
    delete spectraAxis;
    delete ws;
  }

  void testConstructor() {
    TS_ASSERT_EQUALS(spectraAxis->title(), "A spectra axis");
    TS_ASSERT(spectraAxis->unit().get());
    for (int i = 0; i < 5; ++i) {
      TS_ASSERT_EQUALS((*spectraAxis)(i), static_cast<double>(i + 1));
    }
  }

  void testClone() {
    Axis *newSpecAxis = spectraAxis->clone(ws);
    TS_ASSERT_DIFFERS(newSpecAxis, spectraAxis);
    delete newSpecAxis;
  }

  void testCloneDifferentLength() {
    Axis *newSpecAxis = spectraAxis->clone(2, ws);
    TS_ASSERT_DIFFERS(newSpecAxis, spectraAxis);
    TS_ASSERT(newSpecAxis->isSpectra());
    TS_ASSERT_EQUALS(newSpecAxis->title(), "A spectra axis");
    TS_ASSERT_EQUALS(newSpecAxis->unit()->unitID(), "Label");
    // Although the 'different length' constructor is still there (for now) it
    // has no effect.
    TS_ASSERT_EQUALS(newSpecAxis->length(), 5);
    TS_ASSERT_EQUALS((*newSpecAxis)(1), 2.0);
    delete newSpecAxis;
  }

  void testTitle() {
    spectraAxis->title() = "something";
    TS_ASSERT_EQUALS(spectraAxis->title(), "something");
  }

  void testUnit() {
    spectraAxis->unit() = UnitFactory::Instance().create("TOF");
    TS_ASSERT_EQUALS(spectraAxis->unit()->unitID(), "TOF");
  }

  void testIsSpectra() { TS_ASSERT(spectraAxis->isSpectra()); }

  void testIsNumeric() { TS_ASSERT(!spectraAxis->isNumeric()); }

  void testIsText() { TS_ASSERT(!spectraAxis->isText()); }

  void testOperatorBrackets() {
    TS_ASSERT_THROWS((*spectraAxis)(-1), const Exception::IndexError &);
    TS_ASSERT_THROWS((*spectraAxis)(5), const Exception::IndexError &);
  }

  void testSpectraNo() {
    TS_ASSERT_THROWS(spectraAxis->spectraNo(-1), const Exception::IndexError &);
    TS_ASSERT_THROWS(spectraAxis->spectraNo(5), const Exception::IndexError &);

    for (int i = 0; i < 5; ++i) {
      TS_ASSERT_EQUALS(spectraAxis->spectraNo(i), i + 1);
      TS_ASSERT_EQUALS((*spectraAxis)(i), i + 1);
    }
  }

  void testIndexOfValue_Treats_Axis_As_Binned() {
    for (int i = 1; i < 6; ++i) {
      // centre in this bin
      TS_ASSERT_EQUALS(i - 1,
                       spectraAxis->indexOfValue(static_cast<double>(i)));
      // value on lower boundary in bin below with exception of first boundary
      // where it is above
      if (i == 1) {
        TS_ASSERT_EQUALS(
            i - 1, spectraAxis->indexOfValue(static_cast<double>(i - 0.5)));
      } else {
        TS_ASSERT_EQUALS(
            i - 2, spectraAxis->indexOfValue(static_cast<double>(i - 0.5)));
      }
      TS_ASSERT_EQUALS(i - 1,
                       spectraAxis->indexOfValue(static_cast<double>(i + 0.5)));
    }
  }

  // --------------------------------------- Failure cases --------------------
  void testIndexOfValue_Throws_out_of_range_error_If_Input_Not_In_Range() {
    TS_ASSERT_THROWS(spectraAxis->indexOfValue(0.49),
                     const std::out_of_range &);
    TS_ASSERT_THROWS(spectraAxis->indexOfValue(20.), const std::out_of_range &);
  }

private:
  WorkspaceTester *ws;
  Axis *spectraAxis;
};

#endif /*SPECTRAAXISTEST_H_*/
