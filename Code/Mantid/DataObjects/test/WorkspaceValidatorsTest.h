#ifndef WORKSPACEVALIDATORSTEST_H_
#define WORKSPACEVALIDATORSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/Workspace2D.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

class WorkspaceValidatorsTest : public CxxTest::TestSuite
{
private:
  WorkspaceUnitValidator<>* unitVal;
  HistogramValidator<>* histVal;
  RawCountValidator<>* rawVal;
  CompositeValidator<> compVal;

  Workspace_sptr ws1;
  Workspace_sptr ws2;

public:
  WorkspaceValidatorsTest()
  {
    unitVal = new WorkspaceUnitValidator<>("Wavelength");
    histVal = new HistogramValidator<>();
    rawVal = new RawCountValidator<>();

    ws1 = Workspace_sptr(new Mantid::DataObjects::Workspace2D);
    ws1->initialize(1,10,9);
    ws2 = Workspace_sptr(new Mantid::DataObjects::Workspace2D);
    ws2->initialize(1,10,10);
    ws2->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
    ws2->isDistribution(true);
  }

  ~WorkspaceValidatorsTest()
  {
    delete unitVal, histVal, rawVal;
  }

  void testCast()
  {
    TS_ASSERT( dynamic_cast<IValidator<Workspace_sptr>* >(unitVal) )
    TS_ASSERT( dynamic_cast<IValidator<Workspace_sptr>* >(histVal) )
    TS_ASSERT( dynamic_cast<IValidator<Workspace_sptr>* >(rawVal) )
  }

  void testWorkspaceUnitValidator()
  {
    TS_ASSERT_THROWS_NOTHING( WorkspaceUnitValidator<> val; )
  }

  void testWorkspaceUnitValidator_getType()
  {
    TS_ASSERT_EQUALS( unitVal->getType(), "workspaceunit" )
  }

  void testWorkspaceUnitValidator_isValid()
  {
    TS_ASSERT( ! unitVal->isValid(ws1) )
    TS_ASSERT( unitVal->isValid(ws2) )
  }

  void testWorkspaceUnitValidator_clone()
  {
    IValidator<Workspace_sptr> *v = unitVal->clone();
    TS_ASSERT_DIFFERS( v, unitVal )
    TS_ASSERT( dynamic_cast<WorkspaceUnitValidator<>*>(v) )
    delete v;
  }

  void testHistogramValidator()
  {
    TS_ASSERT_THROWS_NOTHING( HistogramValidator<> val(false) )
  }

  void testHistogramValidator_getType()
  {
    TS_ASSERT_EQUALS( histVal->getType(), "histogram" )
  }

  void testHistogramValidator_isValid()
  {
    TS_ASSERT( histVal->isValid(ws1) )
    TS_ASSERT( ! histVal->isValid(ws2) )
    HistogramValidator<> reverse(false);
    TS_ASSERT( ! reverse.isValid(ws1) )
    TS_ASSERT( reverse.isValid(ws2) )
  }

  void testHistogramValidator_clone()
  {
    IValidator<Workspace_sptr> *v = histVal->clone();
    TS_ASSERT_DIFFERS( v, histVal )
    TS_ASSERT( dynamic_cast<HistogramValidator<>*>(v) )
    delete v;
  }

  void testRawCountValidator_getType()
  {
    TS_ASSERT_EQUALS( rawVal->getType(), "rawcount" )
  }

  void testRawCountValidator_isValid()
  {
    TS_ASSERT( rawVal->isValid(ws1) )
    TS_ASSERT( ! rawVal->isValid(ws2) )
  }

  void testRawCountValidator_clone()
  {
    IValidator<Workspace_sptr> *v = rawVal->clone();
    TS_ASSERT_DIFFERS( v, rawVal )
    TS_ASSERT( dynamic_cast<RawCountValidator<>*>(v) )
    delete v;
  }

  void testCompositeValidator_getType()
  {
    TS_ASSERT_EQUALS( compVal.getType(), "composite" )
  }

  void testCompositeValidator_clone()
  {
    IValidator<Workspace_sptr> *v = compVal.clone();
    TS_ASSERT_DIFFERS( v, &compVal )
    TS_ASSERT( dynamic_cast<CompositeValidator<>*>(v) )
  }

  void testCompositeValidator_isValidandAdd()
  {
    // Passes if empty
    TS_ASSERT( compVal.isValid(ws1) )
    TS_ASSERT( compVal.isValid(ws2) )
    compVal.add(unitVal->clone());
    TS_ASSERT( ! compVal.isValid(ws1) )
    TS_ASSERT( compVal.isValid(ws2) )
    CompositeValidator<> compVal2;
    compVal2.add(histVal->clone());
    TS_ASSERT( compVal2.isValid(ws1) )
    TS_ASSERT( ! compVal2.isValid(ws2) )
    compVal2.add(rawVal->clone());
    TS_ASSERT( compVal2.isValid(ws1) )
    TS_ASSERT( ! compVal2.isValid(ws2) )
    compVal2.add(unitVal->clone());
    TS_ASSERT( ! compVal2.isValid(ws1) )
    TS_ASSERT( ! compVal2.isValid(ws2) )

    IValidator<Workspace_sptr>* compVal3 = compVal.clone();
    TS_ASSERT( ! compVal3->isValid(ws1) )
    TS_ASSERT( compVal3->isValid(ws2) )
    delete compVal3;
  }

};

#endif /*WORKSPACEVALIDATORSTEST_H_*/
