#ifndef MANTID_DATAOBJECTS_SPECIALWORKSPACE2DTEST_H_
#define MANTID_DATAOBJECTS_SPECIALWORKSPACE2DTEST_H_

#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/WorkspaceProperty.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid;

class SpecialWorkspace2DTest : public CxxTest::TestSuite
{
public:


  void test_default_constructor()
  {
    SpecialWorkspace2D_sptr ws(new SpecialWorkspace2D());
    TSM_ASSERT_THROWS_ANYTHING("Can't init with > 1 X or Y entries.",  ws->initialize(100, 2, 1));
    TSM_ASSERT_THROWS_ANYTHING("Can't init with > 1 X or Y entries.",  ws->initialize(100, 1, 2));
    TS_ASSERT_THROWS_NOTHING( ws->initialize(100, 1, 1) );

    TS_ASSERT_EQUALS( ws->getNumberHistograms(), 100);
    TS_ASSERT_EQUALS( ws->blocksize(), 1);
  }

  void test_constructor_from_Instrument()
  {
    // Fake instrument with 5*9 pixels with ID starting at 1
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    SpecialWorkspace2D_sptr ws(new SpecialWorkspace2D(inst));

    TS_ASSERT_EQUALS( ws->getNumberHistograms(), 45);
    TS_ASSERT_EQUALS( ws->blocksize(), 1);
    TS_ASSERT_EQUALS( ws->getInstrument()->getName(), "basic"); // Name of the test instrument
    const std::set<detid_t> & dets = ws->getSpectrum(0)->getDetectorIDs();
    TS_ASSERT_EQUALS(dets.size(), 1);

    TS_ASSERT_EQUALS( *(ws->getDetectorIDs(0).begin()), 1);
    TS_ASSERT_EQUALS( *(ws->getDetectorIDs(1).begin()), 2);
  }

  void test_setValue_getValue()
  {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    SpecialWorkspace2D_sptr ws(new SpecialWorkspace2D(inst));

    TS_ASSERT_DIFFERS( ws->getValue(1), 12.3 );
    TS_ASSERT_THROWS_NOTHING( ws->setValue(1, 12.3) );
    TS_ASSERT_DELTA( ws->getValue(1), 12.3, 1e-6 );
    TS_ASSERT_THROWS_ANYTHING( ws->setValue(46, 789) );
    TS_ASSERT_THROWS_ANYTHING( ws->setValue(-1, 789) );
    TS_ASSERT_THROWS_ANYTHING( ws->getValue(47) );
    TS_ASSERT_THROWS_ANYTHING( ws->getValue(-34) );
    TS_ASSERT_EQUALS( ws->getValue(47, 5.0), 5.0 );
    TS_ASSERT_EQUALS( ws->getValue(147, -12.0), -12.0 );

  }

  void test_binaryOperator(){
    Instrument_sptr inst1 = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    SpecialWorkspace2D_sptr ws1(new SpecialWorkspace2D(inst1));

    Instrument_sptr inst2 = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    SpecialWorkspace2D_sptr ws2raw(new SpecialWorkspace2D(inst2));
    SpecialWorkspace2D_const_sptr ws2 = boost::dynamic_pointer_cast<const SpecialWorkspace2D>(ws2raw);

    // 1. AND operation
    ws1->setValue(2, 1);
    ws2raw->setValue(2, 1);
    ws1->binaryOperation(ws2, BinaryOperator::AND);
    TS_ASSERT_EQUALS( ws1->getValue(2), 2);

    ws1->setValue(2, 0);
    ws2raw->setValue(2, 1);
    ws1->binaryOperation(ws2, BinaryOperator::AND);
    TS_ASSERT_EQUALS( ws1->getValue(2), 0);

    ws1->setValue(2, 1);
    ws2raw->setValue(2, 0);
    ws1->binaryOperation(ws2, BinaryOperator::AND);
    TS_ASSERT_EQUALS( ws1->getValue(2), 0);

    ws1->setValue(2, 0);
    ws2raw->setValue(2, 0);
    ws1->binaryOperation(ws2, BinaryOperator::AND);
    TS_ASSERT_EQUALS( ws1->getValue(2), 0);

    // 2. OR operation
    ws1->setValue(2, 1);
    ws2raw->setValue(2, 1);
    ws1->binaryOperation(ws2, BinaryOperator::OR);
    TS_ASSERT_EQUALS( ws1->getValue(2), 1);

    ws1->setValue(2, 0);
    ws2raw->setValue(2, 1);
    ws1->binaryOperation(ws2, BinaryOperator::OR);
    TS_ASSERT_EQUALS( ws1->getValue(2), 1);

    ws1->setValue(2, 1);
    ws2raw->setValue(2, 0);
    ws1->binaryOperation(ws2, BinaryOperator::OR);
    TS_ASSERT_EQUALS( ws1->getValue(2), 1);

    ws1->setValue(2, 0);
    ws2raw->setValue(2, 0);
    ws1->binaryOperation(ws2, BinaryOperator::OR);
    TS_ASSERT_EQUALS( ws1->getValue(2), 0);

    // 3. XOR operation
    // 2. OR operation
    ws1->setValue(2, 1);
    ws2raw->setValue(2, 1);
    ws1->binaryOperation(ws2, BinaryOperator::XOR);
    TS_ASSERT_EQUALS( ws1->getValue(2), 0);

    ws1->setValue(2, 0);
    ws2raw->setValue(2, 1);
    ws1->binaryOperation(ws2, BinaryOperator::XOR);
    TS_ASSERT_EQUALS( ws1->getValue(2), 1);

    ws1->setValue(2, 1);
    ws2raw->setValue(2, 0);
    ws1->binaryOperation(ws2, BinaryOperator::XOR);
    TS_ASSERT_EQUALS( ws1->getValue(2), 1);

    ws1->setValue(2, 0);
    ws2raw->setValue(2, 0);
    ws1->binaryOperation(ws2, BinaryOperator::XOR);
    TS_ASSERT_EQUALS( ws1->getValue(2), 0);

  }

  void test_checkcompatible(){
     Instrument_sptr inst1 = ComponentCreationHelper::createTestInstrumentCylindrical(5);
     SpecialWorkspace2D_sptr ws1(new SpecialWorkspace2D(inst1));

     Instrument_sptr inst2 = ComponentCreationHelper::createTestInstrumentCylindrical(6);
     SpecialWorkspace2D_sptr ws2(new SpecialWorkspace2D(inst2));

     // 1. AND operation
     ws1->setValue(2, 1);
     ws2->setValue(2, 1);

     SpecialWorkspace2D_const_sptr cws2 = boost::dynamic_pointer_cast<const SpecialWorkspace2D>(ws2);
     TS_ASSERT_THROWS_ANYTHING( ws1->binaryOperation(cws2, BinaryOperator::AND) );

  }

  void test_binaryNOT(){
    Instrument_sptr inst1 = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    SpecialWorkspace2D_sptr ws1(new SpecialWorkspace2D(inst1));

    Instrument_sptr inst2 = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    SpecialWorkspace2D_sptr ws2(new SpecialWorkspace2D(inst2));

    Instrument_sptr inst3 = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    SpecialWorkspace2D_sptr ws3(new SpecialWorkspace2D(inst3));

    ws2->binaryOperation(BinaryOperator::NOT);
    SpecialWorkspace2D_const_sptr cws2 = boost::dynamic_pointer_cast<const SpecialWorkspace2D>(ws2);

    ws1->binaryOperation(cws2, BinaryOperator::AND);
    ws3->binaryOperation(cws2, BinaryOperator::OR);

    for (size_t i = 0; i < ws1->getNumberHistograms(); i ++){
      detid_t did = *(ws1->getDetectorIDs(i).begin());
      TS_ASSERT_EQUALS(ws1->getValue(did), 0);
      TS_ASSERT_EQUALS(ws3->getValue(did), 1);
    }
  }

  void test_known_to_property_for_unmangling()
  {
    Mantid::API::WorkspaceProperty<DataObjects::SpecialWorkspace2D> property("DummyProperty", "DummyWorkspace", Mantid::Kernel::Direction::Input);
    TS_ASSERT_EQUALS("SpecialWorkspace2D", Mantid::Kernel::getUnmangledTypeName(*property.type_info()));
  }

  void test_InfoNode()
  {
    SpecialWorkspace2D_sptr ws(new SpecialWorkspace2D());
    ws->initialize(100, 1, 1);

    Mantid::API::Workspace::InfoNode rootNode( *ws );
    ws->addInfoNodeTo( rootNode );
    auto &node = *rootNode.nodes()[0];
    TS_ASSERT_EQUALS( node.nodes().size(), 0 );
    TS_ASSERT_EQUALS( node.lines().size(), 3 );
    TS_ASSERT_EQUALS( node.lines()[0], "SpecialWorkspace2D" );
    TS_ASSERT_EQUALS( node.lines()[1], "Title: " );
    TS_ASSERT_EQUALS( node.lines()[2], "Histograms: 100" );
  }
};


#endif /* MANTID_DATAOBJECTS_SPECIALWORKSPACE2DTEST_H_ */

