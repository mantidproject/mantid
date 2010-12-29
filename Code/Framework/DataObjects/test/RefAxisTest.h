//-------------------------------------------------------------------------------------------------
// This set of tests has been placed in DataObjects because it really needs to use a real workspace
//-------------------------------------------------------------------------------------------------
#ifndef REFAXISTEST_H_
#define REFAXISTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/RefAxis.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidDataObjects/Workspace2D.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class RefAxisTest : public CxxTest::TestSuite
{
public:
  RefAxisTest()
  {
    // Set up two small workspaces for these tests
    space = new Mantid::DataObjects::Workspace2D;
    space->initialize(5,25,25);
    space2 = new Mantid::DataObjects::Workspace2D;
    space2->initialize(1,5,5);

    // Fill them
    double *a = new double[25];
    double *b = new double[25];
    for (int i = 0; i < 25; ++i)
    {
      a[i]=i+0.1;
    }
    for (int j = 0; j < 5; ++j) {
      space->dataX(j) = Mantid::MantidVec(a+(5*j), a+(5*j)+5);
    }
    delete[] a;
    delete[] b;
    
    // Create the axis that the tests will be performed on
    refAxis = new RefAxis(5, space);
    refAxis->title() = "test axis";
    refAxis->unit() = UnitFactory::Instance().create("TOF");
  }
  
  ~RefAxisTest()
  {
    delete refAxis;
    delete space;
    delete space2;
  }
  
  void testConstructor()
  {
    TS_ASSERT_EQUALS( refAxis->title(), "test axis" )
    TS_ASSERT( refAxis->isNumeric() )
    TS_ASSERT( ! refAxis->isSpectra() )
    TS_ASSERT_EQUALS( refAxis->unit()->unitID(), "TOF" )
    TS_ASSERT_THROWS( refAxis->spectraNo(0), std::domain_error )
  }

  void testClone()
  {
    Axis *clonedAxis = refAxis->clone(space2);
    TS_ASSERT_DIFFERS( clonedAxis, refAxis )
    TS_ASSERT( dynamic_cast<RefAxis*>(clonedAxis) )
    TS_ASSERT_EQUALS( clonedAxis->title(), "test axis" )
    TS_ASSERT_EQUALS( clonedAxis->unit()->unitID(), "TOF" )
    TS_ASSERT( clonedAxis->isNumeric() )
    TS_ASSERT_EQUALS( (*clonedAxis)(0,0), 0.0 )
    TS_ASSERT_THROWS( (*clonedAxis)(0,1), std::range_error )
    delete clonedAxis;
  }

  void testOperatorBrackets()
  {
    TS_ASSERT_EQUALS( (*refAxis)(4,4), 24.1 )
    TS_ASSERT_EQUALS( (*refAxis)(0,2), 10.1 )
    TS_ASSERT_EQUALS( (*refAxis)(2,0), 2.1 )
    
    TS_ASSERT_THROWS( (*refAxis)(-1,0), Exception::IndexError )
    TS_ASSERT_THROWS( (*refAxis)(5,0), Exception::IndexError )
    TS_ASSERT_THROWS( (*refAxis)(0,-1), std::range_error )
    TS_ASSERT_THROWS( (*refAxis)(0,5), std::range_error )
  }

  void testSetValue()
  {
    TS_ASSERT_THROWS( refAxis->setValue(0,9.9), std::domain_error )
  }

private:
  MatrixWorkspace *space, *space2;
  RefAxis *refAxis;
};

#endif /*REFAXISTEST_H_*/
