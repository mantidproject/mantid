// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/TextAxis.h"
#include "MantidFrameworkTestHelpers/FakeObjects.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

// Now the unit test class itself
class TextAxisTest : public CxxTest::TestSuite {
public:
  void testConstructor() {
    TextAxis ta(3);
    TS_ASSERT_EQUALS(ta.length(), 3);
    TS_ASSERT(ta.unit());
    TS_ASSERT_EQUALS(ta(0), Mantid::EMPTY_DBL());
    TS_ASSERT_THROWS(ta.setValue(0, 10.), const std::domain_error &);
    TS_ASSERT(ta.isText());
  }

  void testLabels() {
    TextAxis ta(3);
    ta.setLabel(0, "First");
    ta.setLabel(1, "Second");
    ta.setLabel(2, "Third");

    TS_ASSERT_EQUALS(ta.label(0), "First");
    TS_ASSERT_EQUALS(ta.label(1), "Second");
    TS_ASSERT_EQUALS(ta.label(2), "Third");
  }

  void testEquals() {
    TextAxis ta1(2);
    ta1.setLabel(0, "First");
    ta1.setLabel(1, "Second");

    TextAxis ta2(2);
    ta2.setLabel(0, "First");
    ta2.setLabel(1, "Second");

    TextAxis ta3(3);
    ta3.setLabel(0, "First");
    ta3.setLabel(1, "Second");
    ta3.setLabel(2, "Third");

    TextAxis ta4(2);
    ta4.setLabel(0, "Second");
    ta4.setLabel(1, "First");

    TS_ASSERT(ta1 == ta2);
    TS_ASSERT(!(ta1 == ta3));
    TS_ASSERT(!(ta2 == ta4));
  }

  void testClone() {
    TextAxis ta1(2);
    ta1.setLabel(0, "First");
    ta1.setLabel(1, "Second");

    WorkspaceTester ws; // Fake workspace to pass to clone
    Axis *a2 = ta1.clone(&ws);
    TS_ASSERT(a2);
    if (a2) {
      TS_ASSERT(ta1 == *a2);
      delete a2;
    }
  }

  void testCloneDifferentLength() {
    Axis *ta = new TextAxis(2);
    ta->title() = "A text axis";
    WorkspaceTester ws; // Fake workspace to pass to clone
    Axis *newTextAxis = ta->clone(1, &ws);
    TS_ASSERT_DIFFERS(newTextAxis, ta);
    TS_ASSERT(newTextAxis->isText());
    TS_ASSERT_EQUALS(newTextAxis->title(), "A text axis");
    TS_ASSERT_EQUALS(newTextAxis->unit()->unitID(), "Empty");
    TS_ASSERT_EQUALS(newTextAxis->length(), 1);
    delete ta;
    delete newTextAxis;
  }

  void test_getMin_when_numeric_entry() {
    TextAxis ta(2);
    ta.setLabel(0, "3.1");
    ta.setLabel(1, "4.2");
    TS_ASSERT_EQUALS(3.1, ta.getMin())
  }

  void test_getMax_when_numeric_entry() {
    TextAxis ta(2);
    ta.setLabel(0, "3.1");
    ta.setLabel(1, "4.2");
    TS_ASSERT_EQUALS(4.2, ta.getMax())
  }

  void test_getMin_when_not_numeric() {
    TextAxis ta(2);
    ta.setLabel(0, "x3.1");
    ta.setLabel(1, "x4.2");
    TS_ASSERT_EQUALS(0, ta.getMin())
  }

  void test_getMax_when_numeric() {
    TextAxis ta(2);
    ta.setLabel(0, "x3.1");
    ta.setLabel(1, "x4.2");
    TS_ASSERT_EQUALS(1, ta.getMax())
  }

  void test_getMinMax_when_mixed_numeric_non_numeric() {
    TextAxis ta(2);
    ta.setLabel(0, "5.1");
    ta.setLabel(1, "x");
    TS_ASSERT_EQUALS(5.1, ta.getMin())
    TS_ASSERT_EQUALS(ta.getMin() + 1, ta.getMax())
  }

  void test_indexOfValue_Returns_Input_As_Index() {
    TextAxis ta(2);
    TS_ASSERT_EQUALS(static_cast<size_t>(1.5), ta.indexOfValue(1.5));
    TS_ASSERT_THROWS(ta.indexOfValue(-1.5), const std::out_of_range &);
    TS_ASSERT_THROWS(ta.indexOfValue(5), const std::out_of_range &);
  }
};
