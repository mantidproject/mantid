#ifndef BINARYOPHELPERTEST_H_
#define BINARYOPHELPERTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "WorkspaceCreationHelper.hh"
#include "MantidAlgorithms/BinaryOpHelper.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TripleRef.h" 
#include "MantidAPI/TripleIterator.h" 
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class BinaryOpHelperTest : public CxxTest::TestSuite
{
public:

  void testcheckSizeCompatability1D1D()
  {
    int sizex = 10;
    // Register the workspace in the data service
    Workspace1D* work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    Workspace1D* work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(20);
    Workspace1D* work_in3 = WorkspaceCreationHelper::Create1DWorkspaceFib(10);
    Workspace1D* work_in4 = WorkspaceCreationHelper::Create1DWorkspaceFib(5);
    Workspace1D* work_in5 = WorkspaceCreationHelper::Create1DWorkspaceFib(3);
    Workspace1D* work_in6 = WorkspaceCreationHelper::Create1DWorkspaceFib(1);
    Workspace1D* work_in7 = WorkspaceCreationHelper::Create1DWorkspaceFib(0);
    BinaryOpHelper helper;
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in2));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in3));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in4));
    TS_ASSERT(!helper.checkSizeCompatability(work_in1,work_in5));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in6));
    TS_ASSERT(!helper.checkSizeCompatability(work_in1,work_in7));

    //clean up
    delete work_in1;
    delete work_in2;
    delete work_in3;
    delete work_in4;
    delete work_in5;
    delete work_in6;
    delete work_in7;
  }

  void testcheckSizeCompatability2D1D()
  {
    // Register the workspace in the data service
    Workspace2D* work_in1 = WorkspaceCreationHelper::Create2DWorkspace(10,10);
    Workspace1D* work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(20);
    Workspace1D* work_in3 = WorkspaceCreationHelper::Create1DWorkspaceFib(10);
    Workspace1D* work_in4 = WorkspaceCreationHelper::Create1DWorkspaceFib(5);
    Workspace1D* work_in5 = WorkspaceCreationHelper::Create1DWorkspaceFib(3);
    Workspace1D* work_in6 = WorkspaceCreationHelper::Create1DWorkspaceFib(1);
    Workspace1D* work_in7 = WorkspaceCreationHelper::Create1DWorkspaceFib(0);
    BinaryOpHelper helper;
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in2));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in3));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in4));
    TS_ASSERT(!helper.checkSizeCompatability(work_in1,work_in5));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in6));
    TS_ASSERT(!helper.checkSizeCompatability(work_in1,work_in7));

    //clean up
    delete work_in1;
    delete work_in2;
    delete work_in3;
    delete work_in4;
    delete work_in5;
    delete work_in6;
    delete work_in7;
  }

  
  void testcheckSizeCompatability2D2D()
  {
    // Register the workspace in the data service
    Workspace2D* work_in1 = WorkspaceCreationHelper::Create2DWorkspace(10,10);
    Workspace2D* work_in2 = WorkspaceCreationHelper::Create2DWorkspace(20,10);
    Workspace2D* work_in3 = WorkspaceCreationHelper::Create2DWorkspace(10,10);
    Workspace2D* work_in4 = WorkspaceCreationHelper::Create2DWorkspace(5,5);
    Workspace2D* work_in5 = WorkspaceCreationHelper::Create2DWorkspace(3,3);
    Workspace2D* work_in6 = WorkspaceCreationHelper::Create2DWorkspace(1,100);
    Workspace2D* work_in7 = WorkspaceCreationHelper::Create2DWorkspace(0,0);
    BinaryOpHelper helper;
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in2));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in3));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in4));
    TS_ASSERT(!helper.checkSizeCompatability(work_in1,work_in5));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in6));
    TS_ASSERT(!helper.checkSizeCompatability(work_in1,work_in7));

    //clean up
    delete work_in1;
    delete work_in2;
    delete work_in3;
    delete work_in4;
    delete work_in5;
    delete work_in6;
    delete work_in7;
  }

  void testcreateOutputWorkspace1D1D()
  {
    // Register the workspace in the data service
    Workspace1D* work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(10);
    Workspace1D* work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(20);
    Workspace1D* work_in3 = WorkspaceCreationHelper::Create1DWorkspaceFib(10);
    Workspace1D* work_in4 = WorkspaceCreationHelper::Create1DWorkspaceFib(5);
    Workspace1D* work_in5 = WorkspaceCreationHelper::Create1DWorkspaceFib(3);
    Workspace1D* work_in6 = WorkspaceCreationHelper::Create1DWorkspaceFib(1);
    Workspace1D* work_in7 = WorkspaceCreationHelper::Create1DWorkspaceFib(0);
    BinaryOpHelper helper;
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in2),work_in1,work_in2);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in3),work_in1,work_in3);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in4),work_in1,work_in4);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in5),work_in1,work_in5);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in6),work_in1,work_in6);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in7),work_in1,work_in7);

    //clean up
    delete work_in1;
    delete work_in2;
    delete work_in3;
    delete work_in4;
    delete work_in5;
    delete work_in6;
    delete work_in7;
  }
  
  void testcreateOutputWorkspace2D1D()
  {
    // Register the workspace in the data service
    Workspace2D* work_in1 = WorkspaceCreationHelper::Create2DWorkspace(5,2);
    Workspace1D* work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(20);
    Workspace1D* work_in3 = WorkspaceCreationHelper::Create1DWorkspaceFib(10);
    Workspace1D* work_in4 = WorkspaceCreationHelper::Create1DWorkspaceFib(5);
    Workspace1D* work_in5 = WorkspaceCreationHelper::Create1DWorkspaceFib(3);
    Workspace1D* work_in6 = WorkspaceCreationHelper::Create1DWorkspaceFib(1);
    Workspace1D* work_in7 = WorkspaceCreationHelper::Create1DWorkspaceFib(0);
    BinaryOpHelper helper;
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in2),work_in1,work_in2);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in3),work_in1,work_in3);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in4),work_in1,work_in4);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in5),work_in1,work_in5);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in6),work_in1,work_in6);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in7),work_in1,work_in7);

    //clean up
    delete work_in1;
    delete work_in2;
    delete work_in3;
    delete work_in4;
    delete work_in5;
    delete work_in6;
    delete work_in7;
  }

 
  void testcreateOutputWorkspace2D2D()
  {
    // Register the workspace in the data service
    Workspace2D* work_in1 = WorkspaceCreationHelper::Create2DWorkspace(10,10);
    Workspace2D* work_in2 = WorkspaceCreationHelper::Create2DWorkspace(20,10);
    Workspace2D* work_in3 = WorkspaceCreationHelper::Create2DWorkspace(10,10);
    Workspace2D* work_in4 = WorkspaceCreationHelper::Create2DWorkspace(5,5);
    Workspace2D* work_in5 = WorkspaceCreationHelper::Create2DWorkspace(3,3);
    Workspace2D* work_in6 = WorkspaceCreationHelper::Create2DWorkspace(1,100);
    Workspace2D* work_in7 = WorkspaceCreationHelper::Create2DWorkspace(0,0);
    BinaryOpHelper helper;
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in2),work_in1,work_in2);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in3),work_in1,work_in3);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in4),work_in1,work_in4);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in5),work_in1,work_in5);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in6),work_in1,work_in6);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in7),work_in1,work_in7);

    //clean up
    delete work_in1;
    delete work_in2;
    delete work_in3;
    delete work_in4;
    delete work_in5;
    delete work_in6;
    delete work_in7;
  }


  void checkOutputWorkspace(Workspace* ws, Workspace* wsIn1,Workspace* wsIn2 ) const
  {
    int targetsize = (wsIn1->size()>wsIn2->size())?wsIn1->size():wsIn2->size();
    TS_ASSERT_EQUALS(ws->size(),targetsize);
    //check they arre all 0
    for(triple_iterator<Workspace> ti(*ws); ti != ti.end(); ++ti)
    {
      TS_ASSERT_THROWS_NOTHING
      (
        TripleRef<double&> tr = *ti;
        TS_ASSERT_DELTA(tr[0],0,0.0001);
        TS_ASSERT_DELTA(tr[1],0,0.0001);
        TS_ASSERT_DELTA(tr[2],0,0.0001);
      )
    }
  }
  
};

#endif /*BINARYOPHELPERTEST_H_*/
