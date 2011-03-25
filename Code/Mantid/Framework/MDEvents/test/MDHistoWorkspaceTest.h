#ifndef MANTID_MDEVENTS_MDHISTOWORKSPACETEST_H_
#define MANTID_MDEVENTS_MDHISTOWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

using namespace Mantid::MDEvents;
using namespace Mantid::Geometry;

class MDHistoWorkspaceTest : public CxxTest::TestSuite
{
public:

  void test_constructor()
  {
    MDHistoDimension_sptr dimX(new MDHistoDimension("X", "x", -10, 10, 5));
    MDHistoDimension_sptr dimY(new MDHistoDimension("Y", "y", -10, 10, 5));
    MDHistoDimension_sptr dimZ(new MDHistoDimension("Z", "z", -10, 10, 5));
    MDHistoDimension_sptr dimT(new MDHistoDimension("T", "t", -10, 10, 5));

    MDHistoWorkspace ws(dimX, dimY, dimZ, dimT);

    TS_ASSERT_EQUALS( ws.getNDimensions(), 4);
    TS_ASSERT_EQUALS( ws.getNPoints(), 5*5*5*5);
    TS_ASSERT_EQUALS( ws.getMemorySize(), 5*5*5*5 * sizeof(double)*2);
    TS_ASSERT_EQUALS( ws.getXDimension(), dimX);
    TS_ASSERT_EQUALS( ws.getYDimension(), dimY);
    TS_ASSERT_EQUALS( ws.getZDimension(), dimZ);
    TS_ASSERT_EQUALS( ws.getTDimension(), dimT);

    // Methods that are not implemented
    TS_ASSERT_THROWS_ANYTHING( ws.getCell(1) );
    TS_ASSERT_THROWS_ANYTHING( ws.getCell(1,2,3,4) );
    TS_ASSERT_THROWS_ANYTHING( ws.getPoint(1) );

    // The values are cleared at the start
    for (size_t i=0; i <  ws.getNPoints(); i++)
    {
      TS_ASSERT_EQUALS( ws.getSignalAt(i), 0.0);
      TS_ASSERT_EQUALS( ws.getErrorAt(i), 0.0);
    }

    // Setting and getting
    ws.setErrorAt(5,1.234);
    ws.setSignalAt(5,2.3456);
    TS_ASSERT_DELTA( ws.getErrorAt(5), 1.234, 1e-5);
    TS_ASSERT_DELTA( ws.getSignalAt(5), 2.3456, 1e-5);

  }


};


#endif /* MANTID_MDEVENTS_MDHISTOWORKSPACETEST_H_ */

