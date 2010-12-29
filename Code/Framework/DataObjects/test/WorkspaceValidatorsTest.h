#ifndef WORKSPACEVALIDATORSTEST_H_
#define WORKSPACEVALIDATORSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/Workspace2D.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

class WorkspaceValidatorsTest : public CxxTest::TestSuite
{
private:
  WorkspaceUnitValidator<>* wavUnitVal;
  WorkspaceUnitValidator<>* anyUnitVal;
  HistogramValidator<>* histVal;
  RawCountValidator<>* rawVal;
  RawCountValidator<>* nonRawVal;
  CommonBinsValidator<>* binVal;
  CompositeValidator<> compVal;

  MatrixWorkspace_sptr ws1;
  MatrixWorkspace_sptr ws2;

public:
  WorkspaceValidatorsTest()
  {
    wavUnitVal = new WorkspaceUnitValidator<>("Wavelength");
    anyUnitVal = new WorkspaceUnitValidator<>("");
    histVal = new HistogramValidator<>();
    rawVal = new RawCountValidator<>();
    nonRawVal = new RawCountValidator<>(false);
    binVal = new CommonBinsValidator<>();

    ws1 = MatrixWorkspace_sptr(new Mantid::DataObjects::Workspace2D);
    ws1->initialize(2,10,9);

    ws2 = MatrixWorkspace_sptr(new Mantid::DataObjects::Workspace2D);
    ws2->initialize(2,10,10);
    ws2->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
    ws2->isDistribution(true);
  }

  ~WorkspaceValidatorsTest()
  {
    delete wavUnitVal, anyUnitVal, histVal, rawVal, nonRawVal, binVal;
  }

  void testCast()
  {
    TS_ASSERT( dynamic_cast<IValidator<MatrixWorkspace_sptr>* >(wavUnitVal) );
    TS_ASSERT( dynamic_cast<IValidator<MatrixWorkspace_sptr>* >(anyUnitVal) );
    TS_ASSERT( dynamic_cast<IValidator<MatrixWorkspace_sptr>* >(histVal) );
    TS_ASSERT( dynamic_cast<IValidator<MatrixWorkspace_sptr>* >(rawVal) );
    TS_ASSERT( dynamic_cast<IValidator<MatrixWorkspace_sptr>* >(nonRawVal) );
    TS_ASSERT( dynamic_cast<IValidator<MatrixWorkspace_sptr>* >(binVal) );
  }

  void testWorkspaceUnitValidator()
  {
    TS_ASSERT_THROWS_NOTHING( WorkspaceUnitValidator<>() );
  }

  void testWorkspaceUnitValidator_getType()
  {
    TS_ASSERT_EQUALS( wavUnitVal->getType(), "workspaceunit" );
    TS_ASSERT_EQUALS( anyUnitVal->getType(), "workspaceunit" );
  }

  void testWorkspaceUnitValidator_isValid()
  {
	  TS_ASSERT_EQUALS( wavUnitVal->isValid(ws1), "The workspace must have units of Wavelength" );
    TS_ASSERT_EQUALS( wavUnitVal->isValid(ws2), "" );
    TS_ASSERT_EQUALS( anyUnitVal->isValid(ws1), "The workspace must have units" );
    TS_ASSERT_EQUALS( anyUnitVal->isValid(ws2), "" );
  }

  void testWorkspaceUnitValidator_clone()
  {
    IValidator<MatrixWorkspace_sptr> *v = wavUnitVal->clone();
    TS_ASSERT_DIFFERS( v, wavUnitVal );
    TS_ASSERT( dynamic_cast<WorkspaceUnitValidator<>*>(v) );
    delete v;
  }

  void testHistogramValidator()
  {
    TS_ASSERT_THROWS_NOTHING( HistogramValidator<>(false) );
  }

  void testHistogramValidator_getType()
  {
    TS_ASSERT_EQUALS( histVal->getType(), "histogram" );
  }

  void testHistogramValidator_isValid()
  {
    TS_ASSERT_EQUALS( histVal->isValid(ws1), "" );
    TS_ASSERT_EQUALS( histVal->isValid(ws2), "The workspace must contain histogram data" );
    HistogramValidator<> reverse(false);
    TS_ASSERT_EQUALS( reverse.isValid(ws1), "The workspace must not contain histogram data" );
    TS_ASSERT_EQUALS( reverse.isValid(ws2), "" );
  }

  void testHistogramValidator_clone()
  {
    IValidator<MatrixWorkspace_sptr> *v = histVal->clone();
    TS_ASSERT_DIFFERS( v, histVal );
    TS_ASSERT( dynamic_cast<HistogramValidator<>*>(v) );
    delete v;
  }

  void testRawCountValidator_getType()
  {
    TS_ASSERT_EQUALS( nonRawVal->getType(), "rawcount" );
  }

  void testRawCountValidator_isValid()
  {
    TS_ASSERT_EQUALS( rawVal->isValid(ws1), "" );
    TS_ASSERT_EQUALS( rawVal->isValid(ws2),
        "A workspace containing numbers of counts is required here" );
    TS_ASSERT_EQUALS( nonRawVal->isValid(ws1),
        "A workspace of numbers of counts is not allowed here" );
    TS_ASSERT_EQUALS( nonRawVal->isValid(ws2), "" );
  }

  void testRawCountValidator_clone()
  {
    IValidator<MatrixWorkspace_sptr> *v = rawVal->clone();
    TS_ASSERT_DIFFERS( v, rawVal );
    TS_ASSERT( dynamic_cast<RawCountValidator<>*>(v) );
    delete v;
  }

  void testCommonBinsValidator_getType()
  {
    TS_ASSERT_EQUALS( binVal->getType(), "commonbins" );
  }

  void testCommonBinsValidator_isValid()
  {
    TS_ASSERT_EQUALS( binVal->isValid(ws1), "" );
    TS_ASSERT_EQUALS( binVal->isValid(ws2), "" );
    ws1->dataX(0)[5] = 0.0;
    TS_ASSERT_EQUALS( binVal->isValid(ws1), "" );
    ws1->dataX(0)[5] = 1.1;
    TS_ASSERT_EQUALS( binVal->isValid(ws1),
        "The workspace must have common bin boundaries for all histograms");
  }

  void testCommonBinsValidator_clone()
  {
    IValidator<MatrixWorkspace_sptr> *v = binVal->clone();
    TS_ASSERT_DIFFERS( v, binVal );
    TS_ASSERT( dynamic_cast<CommonBinsValidator<>*>(v) );
    delete v;
  }

  void testCompositeValidator_getType()
  {
    TS_ASSERT_EQUALS( compVal.getType(), "composite" );
  }

  void testCompositeValidator_clone()
  {
    IValidator<MatrixWorkspace_sptr> *v = compVal.clone();
    TS_ASSERT_DIFFERS( v, &compVal );
    TS_ASSERT( dynamic_cast<CompositeValidator<>*>(v) );
    delete v;
  }

  void testCompositeValidator_isValidandAdd()
  {
    // Passes if empty
    TS_ASSERT_EQUALS( compVal.isValid(ws1), "" );
    TS_ASSERT_EQUALS( compVal.isValid(ws2), "" );
    compVal.add(wavUnitVal->clone());
    TS_ASSERT_EQUALS( compVal.isValid(ws1),
        "The workspace must have units of Wavelength");
    TS_ASSERT_EQUALS( compVal.isValid(ws2), "" );

    CompositeValidator<> compVal2;
    compVal2.add(histVal->clone());
    TS_ASSERT_EQUALS( compVal2.isValid(ws1), "" );
    TS_ASSERT_EQUALS( compVal2.isValid(ws2),
        "The workspace must contain histogram data" );
    compVal2.add(rawVal->clone());
    TS_ASSERT_EQUALS( compVal2.isValid(ws1), "" );
    TS_ASSERT_EQUALS( compVal2.isValid(ws2),
        "The workspace must contain histogram data" );
    compVal2.add(anyUnitVal->clone());
    TS_ASSERT_EQUALS( compVal2.isValid(ws1),
        "The workspace must have units" );
    TS_ASSERT_EQUALS( compVal2.isValid(ws2),
        "The workspace must contain histogram data" );

    IValidator<MatrixWorkspace_sptr>* compVal3 = compVal.clone();
    TS_ASSERT_EQUALS( compVal3->isValid(ws1),
        "The workspace must have units of Wavelength");
    TS_ASSERT_EQUALS( compVal3->isValid(ws2), "" );
    delete compVal3;
  }

  void testWSPropertyandValidator()
  {
      
    WorkspaceProperty<MatrixWorkspace> wsp1("workspace1","ws1",Direction::Input,wavUnitVal->clone());
    //test property validation    
    TS_ASSERT_EQUALS( wsp1.isValid(), "Workspace \"ws1\" was not found in the Analysis Data Service" );;
    
    TS_ASSERT_EQUALS( wsp1.setValue(""),  "Enter a name for the Input/InOut workspace" );
    
    //fine and correct unit
    wsp1 = ws2;
    TS_ASSERT_EQUALS( wsp1.isValid(), "" );;

    //fine and no unit
    TS_ASSERT_THROWS( wsp1 = ws1, std::invalid_argument);

    TS_ASSERT_EQUALS( wsp1.setValue(""),  "Enter a name for the Input/InOut workspace" );
    TS_ASSERT_EQUALS( wsp1.isValid(), "Enter a name for the Input/InOut workspace" );
  }

  void testInstrumentValidator()
  {
    IValidator<MatrixWorkspace_sptr> * instVal = new InstrumentValidator<>();
    MatrixWorkspace_sptr ws = MatrixWorkspace_sptr(new Mantid::DataObjects::Workspace2D);

    // Fails if no instrument
    TS_ASSERT_EQUALS( instVal->isValid(ws), "The workspace must have an instrument defined" );

    // Add a sample pos and then things will be fine
    Mantid::Geometry::ObjComponent * sample = new Mantid::Geometry::ObjComponent("Sample");
    ws->getBaseInstrument()->add(sample);  // This takes care of deletion
    ws->getBaseInstrument()->markAsSamplePos(sample);
    TS_ASSERT_EQUALS( instVal->isValid(ws), "" );

    delete instVal;
  }

};

#endif /*WORKSPACEVALIDATORSTEST_H_*/
