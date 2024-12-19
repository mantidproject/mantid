// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/Timer.h"
#include "PropertyManagerHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid;

class SpecialWorkspace2DTest : public CxxTest::TestSuite {
public:
  void test_default_constructor() {
    SpecialWorkspace2D_sptr ws(new SpecialWorkspace2D());
    TSM_ASSERT_THROWS_ANYTHING("Can't init with > 1 X or Y entries.", ws->initialize(100, 2, 1));
    TSM_ASSERT_THROWS_ANYTHING("Can't init with > 1 X or Y entries.", ws->initialize(100, 1, 2));
    TS_ASSERT_THROWS_NOTHING(ws->initialize(100, 1, 1));

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 100);
    TS_ASSERT_EQUALS(ws->blocksize(), 1);
  }

  void test_empty_detID_map() {
    // Create and initialize a workspace without an instrument
    SpecialWorkspace2D_sptr ws(new SpecialWorkspace2D());
    TS_ASSERT_THROWS_NOTHING(ws->initialize(1, 1, 1));
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(ws->blocksize(), 1);

    // Confirm that the detector ID map is empty
    TS_ASSERT(ws->isDetectorIDMappingEmpty());

    // Set a detector ID for the spectrum. Confirm that we can't get/set value for that detector ID
    TS_ASSERT_THROWS_NOTHING(ws->getSpectrum(0).setDetectorID(0));
    TSM_ASSERT_THROWS_ANYTHING("Can't get value for detector ID=0", ws->getValue(0));
    TSM_ASSERT_THROWS_ANYTHING("Can't set value for detector ID=0", ws->setValue(0, 0, 0));

    // Build the detector ID map. Confirm that we now can get/set value for that detector ID
    TS_ASSERT_THROWS_NOTHING(ws->buildDetectorIDMapping());
    TS_ASSERT_THROWS_NOTHING(ws->getValue(0));
    TS_ASSERT_THROWS_NOTHING(ws->setValue(0, 0, 0));
  }

  void testClone() {
    // As test_setValue_getValue, set on ws, get on clone.
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    SpecialWorkspace2D_sptr ws(new SpecialWorkspace2D(inst));

    auto cloned = ws->clone();
    TS_ASSERT_DIFFERS(cloned->getValue(1), 12.3);
    TS_ASSERT_THROWS_NOTHING(ws->setValue(1, 12.3));
    cloned = ws->clone();
    TS_ASSERT_DELTA(cloned->getValue(1), 12.3, 1e-6);
    TS_ASSERT_THROWS_ANYTHING(ws->setValue(46, 789));
    TS_ASSERT_THROWS_ANYTHING(ws->setValue(-1, 789));
    cloned = ws->clone();
    TS_ASSERT_THROWS_ANYTHING(cloned->getValue(47));
    TS_ASSERT_THROWS_ANYTHING(cloned->getValue(-34));
    TS_ASSERT_EQUALS(cloned->getValue(47, 5.0), 5.0);
    TS_ASSERT_EQUALS(cloned->getValue(147, -12.0), -12.0);

    // Some extra tests: 1. clone ws, 2. set on ws, 3. clone should differ.
    cloned = ws->clone();
    TS_ASSERT_DELTA(cloned->getValue(1), 12.3, 1e-6);
    TS_ASSERT_THROWS_NOTHING(ws->setValue(1, 1.1));
    TS_ASSERT_DIFFERS(cloned->getValue(1), 1.1);
  }

  void test_constructor_from_Instrument() {
    // Fake instrument with 5*9 pixels with ID starting at 1
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    SpecialWorkspace2D_sptr ws(new SpecialWorkspace2D(inst));

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 45);
    TS_ASSERT_EQUALS(ws->blocksize(), 1);
    TS_ASSERT_EQUALS(ws->getInstrument()->getName(),
                     "basic"); // Name of the test instrument
    const auto &dets = ws->getSpectrum(0).getDetectorIDs();
    TS_ASSERT_EQUALS(dets.size(), 1);

    TS_ASSERT_EQUALS(*(ws->getDetectorIDs(0).begin()), 1);
    TS_ASSERT_EQUALS(*(ws->getDetectorIDs(1).begin()), 2);
  }

  void test_setValue_getValue() {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    SpecialWorkspace2D_sptr ws(new SpecialWorkspace2D(inst));

    TS_ASSERT_DIFFERS(ws->getValue(1), 12.3);
    TS_ASSERT_THROWS_NOTHING(ws->setValue(1, 12.3));
    TS_ASSERT_DELTA(ws->getValue(1), 12.3, 1e-6);
    TS_ASSERT_THROWS_ANYTHING(ws->setValue(46, 789));
    TS_ASSERT_THROWS_ANYTHING(ws->setValue(-1, 789));
    TS_ASSERT_THROWS_ANYTHING(ws->getValue(47));
    TS_ASSERT_THROWS_ANYTHING(ws->getValue(-34));
    TS_ASSERT_EQUALS(ws->getValue(47, 5.0), 5.0);
    TS_ASSERT_EQUALS(ws->getValue(147, -12.0), -12.0);
  }

  void test_binaryOperator() {
    Instrument_sptr inst1 = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    SpecialWorkspace2D_sptr ws1(new SpecialWorkspace2D(inst1));

    Instrument_sptr inst2 = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    SpecialWorkspace2D_sptr ws2raw(new SpecialWorkspace2D(inst2));
    SpecialWorkspace2D_const_sptr ws2 = std::dynamic_pointer_cast<const SpecialWorkspace2D>(ws2raw);

    // 1. AND operation
    ws1->setValue(2, 1);
    ws2raw->setValue(2, 1);
    ws1->binaryOperation(ws2, BinaryOperator::AND);
    TS_ASSERT_EQUALS(ws1->getValue(2), 2);

    ws1->setValue(2, 0);
    ws2raw->setValue(2, 1);
    ws1->binaryOperation(ws2, BinaryOperator::AND);
    TS_ASSERT_EQUALS(ws1->getValue(2), 0);

    ws1->setValue(2, 1);
    ws2raw->setValue(2, 0);
    ws1->binaryOperation(ws2, BinaryOperator::AND);
    TS_ASSERT_EQUALS(ws1->getValue(2), 0);

    ws1->setValue(2, 0);
    ws2raw->setValue(2, 0);
    ws1->binaryOperation(ws2, BinaryOperator::AND);
    TS_ASSERT_EQUALS(ws1->getValue(2), 0);

    // 2. OR operation
    ws1->setValue(2, 1);
    ws2raw->setValue(2, 1);
    ws1->binaryOperation(ws2, BinaryOperator::OR);
    TS_ASSERT_EQUALS(ws1->getValue(2), 1);

    ws1->setValue(2, 0);
    ws2raw->setValue(2, 1);
    ws1->binaryOperation(ws2, BinaryOperator::OR);
    TS_ASSERT_EQUALS(ws1->getValue(2), 1);

    ws1->setValue(2, 1);
    ws2raw->setValue(2, 0);
    ws1->binaryOperation(ws2, BinaryOperator::OR);
    TS_ASSERT_EQUALS(ws1->getValue(2), 1);

    ws1->setValue(2, 0);
    ws2raw->setValue(2, 0);
    ws1->binaryOperation(ws2, BinaryOperator::OR);
    TS_ASSERT_EQUALS(ws1->getValue(2), 0);

    // 3. XOR operation
    // 2. OR operation
    ws1->setValue(2, 1);
    ws2raw->setValue(2, 1);
    ws1->binaryOperation(ws2, BinaryOperator::XOR);
    TS_ASSERT_EQUALS(ws1->getValue(2), 0);

    ws1->setValue(2, 0);
    ws2raw->setValue(2, 1);
    ws1->binaryOperation(ws2, BinaryOperator::XOR);
    TS_ASSERT_EQUALS(ws1->getValue(2), 1);

    ws1->setValue(2, 1);
    ws2raw->setValue(2, 0);
    ws1->binaryOperation(ws2, BinaryOperator::XOR);
    TS_ASSERT_EQUALS(ws1->getValue(2), 1);

    ws1->setValue(2, 0);
    ws2raw->setValue(2, 0);
    ws1->binaryOperation(ws2, BinaryOperator::XOR);
    TS_ASSERT_EQUALS(ws1->getValue(2), 0);
  }

  void test_checkcompatible() {
    Instrument_sptr inst1 = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    SpecialWorkspace2D_sptr ws1(new SpecialWorkspace2D(inst1));

    Instrument_sptr inst2 = ComponentCreationHelper::createTestInstrumentCylindrical(6);
    SpecialWorkspace2D_sptr ws2(new SpecialWorkspace2D(inst2));

    // 1. AND operation
    ws1->setValue(2, 1);
    ws2->setValue(2, 1);

    SpecialWorkspace2D_const_sptr cws2 = std::dynamic_pointer_cast<const SpecialWorkspace2D>(ws2);
    TS_ASSERT_THROWS_ANYTHING(ws1->binaryOperation(cws2, BinaryOperator::AND));
  }

  void test_binaryNOT() {
    Instrument_sptr inst1 = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    SpecialWorkspace2D_sptr ws1(new SpecialWorkspace2D(inst1));

    Instrument_sptr inst2 = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    SpecialWorkspace2D_sptr ws2(new SpecialWorkspace2D(inst2));

    Instrument_sptr inst3 = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    SpecialWorkspace2D_sptr ws3(new SpecialWorkspace2D(inst3));

    ws2->binaryOperation(BinaryOperator::NOT);
    SpecialWorkspace2D_const_sptr cws2 = std::dynamic_pointer_cast<const SpecialWorkspace2D>(ws2);

    ws1->binaryOperation(cws2, BinaryOperator::AND);
    ws3->binaryOperation(cws2, BinaryOperator::OR);

    for (size_t i = 0; i < ws1->getNumberHistograms(); i++) {
      detid_t did = *(ws1->getDetectorIDs(i).begin());
      TS_ASSERT_EQUALS(ws1->getValue(did), 0);
      TS_ASSERT_EQUALS(ws3->getValue(did), 1);
    }
  }

  void test_known_to_property_for_unmangling() {
    Mantid::API::WorkspaceProperty<DataObjects::SpecialWorkspace2D> property("DummyProperty", "DummyWorkspace",
                                                                             Mantid::Kernel::Direction::Input);
    TS_ASSERT_EQUALS("SpecialWorkspace2D", Mantid::Kernel::getUnmangledTypeName(*property.type_info()));
  }

  /**
   * Test declaring an input SpecialWorkspace2D and retrieving it as const_sptr
   * or sptr
   */
  void testGetProperty_const_sptr() {
    const std::string wsName = "InputWorkspace";
    SpecialWorkspace2D_sptr wsInput(new SpecialWorkspace2D());
    PropertyManagerHelper manager;
    manager.declareProperty(wsName, wsInput, Mantid::Kernel::Direction::Input);

    // Check property can be obtained as const_sptr or sptr
    SpecialWorkspace2D_const_sptr wsConst;
    SpecialWorkspace2D_sptr wsNonConst;
    TS_ASSERT_THROWS_NOTHING(wsConst = manager.getValue<SpecialWorkspace2D_const_sptr>(wsName));
    TS_ASSERT(wsConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsNonConst = manager.getValue<SpecialWorkspace2D_sptr>(wsName));
    TS_ASSERT(wsNonConst != nullptr);
    TS_ASSERT_EQUALS(wsConst, wsNonConst);

    // Check TypedValue can be cast to const_sptr or to sptr
    PropertyManagerHelper::TypedValue val(manager, wsName);
    SpecialWorkspace2D_const_sptr wsCastConst;
    SpecialWorkspace2D_sptr wsCastNonConst;
    TS_ASSERT_THROWS_NOTHING(wsCastConst = (SpecialWorkspace2D_const_sptr)val);
    TS_ASSERT(wsCastConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsCastNonConst = (SpecialWorkspace2D_sptr)val);
    TS_ASSERT(wsCastNonConst != nullptr);
    TS_ASSERT_EQUALS(wsCastConst, wsCastNonConst);
  }
};
