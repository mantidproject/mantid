#ifndef COMMUTATIVEBINARYOPERATIONTEST_H_
#define COMMUTATIVEBINARYOPERATIONTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "WorkspaceCreationHelper.hh"
#include "MantidAlgorithms/CommutativeBinaryOperation.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TripleIterator.h" 
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

 class CommutativeBinaryOpHelper : public CommutativeBinaryOperation
    {
    public:
      /// Default constructor
      CommutativeBinaryOpHelper() : CommutativeBinaryOperation() {};
      /// Destructor
      virtual ~CommutativeBinaryOpHelper() {};
      const bool checkSizeCompatability(const Workspace_sptr ws1,const Workspace_sptr ws2) const
      {
        return CommutativeBinaryOperation::checkSizeCompatability(ws1,ws2);
      }
      Workspace_sptr createOutputWorkspace(const Workspace_sptr ws1, const Workspace_sptr ws2) const
      {
        return CommutativeBinaryOperation::createOutputWorkspace(ws1,ws2);
      }



    private:
      // Overridden BinaryOperation methods
      void performBinaryOperation(Workspace::const_iterator it_in1, Workspace::const_iterator it_in2,
        Workspace::iterator it_out)
      {}
    };

class CommutativeBinaryOperationTest : public CxxTest::TestSuite
{
public:

  void testcheckSizeCompatability1D1D()
  {
    int sizex = 10;
    // Register the workspace in the data service
    Workspace1D_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    Workspace1D_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(20);
    Workspace1D_sptr work_in3 = WorkspaceCreationHelper::Create1DWorkspaceFib(10);
    Workspace1D_sptr work_in4 = WorkspaceCreationHelper::Create1DWorkspaceFib(5);
    Workspace1D_sptr work_in5 = WorkspaceCreationHelper::Create1DWorkspaceFib(3);
    Workspace1D_sptr work_in6 = WorkspaceCreationHelper::Create1DWorkspaceFib(1);
    Workspace1D_sptr work_in7 = WorkspaceCreationHelper::Create1DWorkspaceFib(0);
    CommutativeBinaryOpHelper helper;
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in2));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in3));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in4));
    TS_ASSERT(!helper.checkSizeCompatability(work_in1,work_in5));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in6));
    TS_ASSERT(!helper.checkSizeCompatability(work_in1,work_in7));
  }

  void testcheckSizeCompatability2D1D()
  {
    // Register the workspace in the data service
    Workspace2D_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(10,10);
    Workspace1D_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(20);
    Workspace1D_sptr work_in3 = WorkspaceCreationHelper::Create1DWorkspaceFib(10);
    Workspace1D_sptr work_in4 = WorkspaceCreationHelper::Create1DWorkspaceFib(5);
    Workspace1D_sptr work_in5 = WorkspaceCreationHelper::Create1DWorkspaceFib(3);
    Workspace1D_sptr work_in6 = WorkspaceCreationHelper::Create1DWorkspaceFib(1);
    Workspace1D_sptr work_in7 = WorkspaceCreationHelper::Create1DWorkspaceFib(0);
    CommutativeBinaryOpHelper helper;
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in2));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in3));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in4));
    TS_ASSERT(!helper.checkSizeCompatability(work_in1,work_in5));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in6));
    TS_ASSERT(!helper.checkSizeCompatability(work_in1,work_in7));
  }

  
  void testcheckSizeCompatability2D2D()
  {
    // Register the workspace in the data service
    Workspace2D_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(10,10);
    Workspace2D_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(20,10);
    Workspace2D_sptr work_in3 = WorkspaceCreationHelper::Create2DWorkspace(10,10);
    Workspace2D_sptr work_in4 = WorkspaceCreationHelper::Create2DWorkspace(5,5);
    Workspace2D_sptr work_in5 = WorkspaceCreationHelper::Create2DWorkspace(3,3);
    Workspace2D_sptr work_in6 = WorkspaceCreationHelper::Create2DWorkspace(1,100);
    Workspace2D_sptr work_in7 = WorkspaceCreationHelper::Create2DWorkspace(0,0);
    CommutativeBinaryOpHelper helper;
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in2));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in3));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in4));
    TS_ASSERT(!helper.checkSizeCompatability(work_in1,work_in5));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in6));
    TS_ASSERT(!helper.checkSizeCompatability(work_in1,work_in7));
  }

  void testcreateOutputWorkspace1D1D()
  {
    // Register the workspace in the data service
    Workspace1D_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(10);
    Workspace1D_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(20);
    Workspace1D_sptr work_in3 = WorkspaceCreationHelper::Create1DWorkspaceFib(10);
    Workspace1D_sptr work_in4 = WorkspaceCreationHelper::Create1DWorkspaceFib(5);
    Workspace1D_sptr work_in5 = WorkspaceCreationHelper::Create1DWorkspaceFib(3);
    Workspace1D_sptr work_in6 = WorkspaceCreationHelper::Create1DWorkspaceFib(1);
    Workspace1D_sptr work_in7 = WorkspaceCreationHelper::Create1DWorkspaceFib(0);
    CommutativeBinaryOpHelper helper;
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in2),work_in1,work_in2);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in3),work_in1,work_in3);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in4),work_in1,work_in4);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in5),work_in1,work_in5);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in6),work_in1,work_in6);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in7),work_in1,work_in7);
  }
  
  void testcreateOutputWorkspace2D1D()
  {
    // Register the workspace in the data service
    Workspace2D_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(5,2);
    Workspace1D_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(20);
    Workspace1D_sptr work_in3 = WorkspaceCreationHelper::Create1DWorkspaceFib(10);
    Workspace1D_sptr work_in4 = WorkspaceCreationHelper::Create1DWorkspaceFib(5);
    Workspace1D_sptr work_in5 = WorkspaceCreationHelper::Create1DWorkspaceFib(3);
    Workspace1D_sptr work_in6 = WorkspaceCreationHelper::Create1DWorkspaceFib(1);
    Workspace1D_sptr work_in7 = WorkspaceCreationHelper::Create1DWorkspaceFib(0);
    CommutativeBinaryOpHelper helper;
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in2),work_in1,work_in2);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in3),work_in1,work_in3);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in4),work_in1,work_in4);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in5),work_in1,work_in5);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in6),work_in1,work_in6);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in7),work_in1,work_in7);
  }

 
  void testcreateOutputWorkspace2D2D()
  {
    // Register the workspace in the data service
    Workspace2D_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(10,10);
    Workspace2D_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(20,10);
    Workspace2D_sptr work_in3 = WorkspaceCreationHelper::Create2DWorkspace(10,10);
    Workspace2D_sptr work_in4 = WorkspaceCreationHelper::Create2DWorkspace(5,5);
    Workspace2D_sptr work_in5 = WorkspaceCreationHelper::Create2DWorkspace(3,3);
    Workspace2D_sptr work_in6 = WorkspaceCreationHelper::Create2DWorkspace(1,100);
    Workspace2D_sptr work_in7 = WorkspaceCreationHelper::Create2DWorkspace(0,0);
    CommutativeBinaryOpHelper helper;
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in2),work_in1,work_in2);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in3),work_in1,work_in3);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in4),work_in1,work_in4);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in5),work_in1,work_in5);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in6),work_in1,work_in6);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in7),work_in1,work_in7);
  }

  void checkOutputWorkspace(Workspace_sptr ws, Workspace_sptr wsIn1,Workspace_sptr wsIn2 ) const
  {
    int targetsize = (wsIn1->size()>wsIn2->size())?wsIn1->size():wsIn2->size();
    TS_ASSERT_EQUALS(ws->size(),targetsize);
    //check they arre all 0
    for(Workspace::iterator ti(*ws); ti != ti.end(); ++ti)
    {
      TS_ASSERT_THROWS_NOTHING
      (
        LocatedDataRef tr = *ti;
        TS_ASSERT_DELTA(tr.X(),0,0.0001);
        TS_ASSERT_DELTA(tr.Y(),0,0.0001);
        TS_ASSERT_DELTA(tr.E(),0,0.0001);
      )
    }
  }
  
};

#endif /*COMMUTATIVEBINARYOPERATIONTEST_H_*/
