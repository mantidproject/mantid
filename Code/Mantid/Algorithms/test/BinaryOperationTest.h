#ifndef BINARYOPERATIONTEST_H_
#define BINARYOPERATIONTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "WorkspaceCreationHelper.hh"
#include "MantidAlgorithms/BinaryOperation.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceIterator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

 class BinaryOpHelper : public BinaryOperation
    {
    public:
      /// Default constructor
      BinaryOpHelper() : BinaryOperation() {};
      /// Destructor
      virtual ~BinaryOpHelper() {};
      const bool checkSizeCompatability(const Workspace_sptr ws1,const Workspace_sptr ws2) const
      {
        return BinaryOperation::checkSizeCompatability(ws1,ws2);
      }
      const bool checkXarrayCompatability(const Workspace_sptr ws1, const Workspace_sptr ws2) const
      {
        return BinaryOperation::checkXarrayCompatability(ws1,ws2);
      }
      const int getRelativeLoopCount(const Workspace_sptr ws1, const Workspace_sptr ws2) const
      {
        return BinaryOperation::getRelativeLoopCount(ws1,ws2);
      }
      Workspace_sptr createOutputWorkspace(const Workspace_sptr ws1, const Workspace_sptr ws2) const
      {
        return BinaryOperation::createOutputWorkspace(ws1,ws2);
      }



    private:
      // Overridden BinaryOperation methods
      void performBinaryOperation(Workspace::const_iterator it_in1, Workspace::const_iterator it_in2,
        Workspace::iterator it_out)
      {}
      /// Static reference to the logger class
      static Mantid::Kernel::Logger& g_log;
    };

class BinaryOperationTest : public CxxTest::TestSuite
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
    BinaryOpHelper helper;
    TS_ASSERT(!helper.checkSizeCompatability(work_in1,work_in2));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in3));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in4));
    TS_ASSERT(!helper.checkSizeCompatability(work_in1,work_in5));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in6));
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
    BinaryOpHelper helper;
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in2));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in3));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in4));
    TS_ASSERT(!helper.checkSizeCompatability(work_in1,work_in5));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in6));
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
    BinaryOpHelper helper;
    TS_ASSERT(!helper.checkSizeCompatability(work_in1,work_in2));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in3));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in4));
    TS_ASSERT(!helper.checkSizeCompatability(work_in1,work_in5));
    TS_ASSERT(helper.checkSizeCompatability(work_in1,work_in6));
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
    BinaryOpHelper helper;
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in2),work_in1,work_in2);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in3),work_in1,work_in3);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in4),work_in1,work_in4);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in5),work_in1,work_in5);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in6),work_in1,work_in6);
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
    BinaryOpHelper helper;
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in2),work_in1,work_in2);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in3),work_in1,work_in3);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in4),work_in1,work_in4);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in5),work_in1,work_in5);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in6),work_in1,work_in6);
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
    BinaryOpHelper helper;
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in2),work_in1,work_in2);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in3),work_in1,work_in3);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in4),work_in1,work_in4);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in5),work_in1,work_in5);
    checkOutputWorkspace(helper.createOutputWorkspace(work_in1,work_in6),work_in1,work_in6);
  }

  void testgetRelativeLoopCount()
  {
    // Register the workspace in the data service
    Workspace1D_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(10);
    Workspace1D_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(20);
    Workspace1D_sptr work_in3 = WorkspaceCreationHelper::Create1DWorkspaceFib(1);
    Workspace2D_sptr work_in4 = WorkspaceCreationHelper::Create2DWorkspace(4,5);
    Workspace2D_sptr work_in5 = WorkspaceCreationHelper::Create2DWorkspace(3,3);
    Workspace2D_sptr work_in6 = WorkspaceCreationHelper::Create2DWorkspace(1,100);
    BinaryOpHelper helper;
    TS_ASSERT_EQUALS(helper.getRelativeLoopCount(work_in1,work_in2),2);
    TS_ASSERT_EQUALS(helper.getRelativeLoopCount(work_in2,work_in1),1);
    TS_ASSERT_EQUALS(helper.getRelativeLoopCount(work_in2,work_in2),1);
    TS_ASSERT_EQUALS(helper.getRelativeLoopCount(work_in1,work_in3),1);
    TS_ASSERT_EQUALS(helper.getRelativeLoopCount(work_in3,work_in1),10);
    TS_ASSERT_EQUALS(helper.getRelativeLoopCount(work_in2,work_in4),1);
    TS_ASSERT_EQUALS(helper.getRelativeLoopCount(work_in4,work_in2),1);
    TS_ASSERT_EQUALS(helper.getRelativeLoopCount(work_in5,work_in3),1);
    TS_ASSERT_EQUALS(helper.getRelativeLoopCount(work_in3,work_in5),9);
    TS_ASSERT_EQUALS(helper.getRelativeLoopCount(work_in6,work_in1),1);
    TS_ASSERT_EQUALS(helper.getRelativeLoopCount(work_in1,work_in6),10);
    TS_ASSERT_EQUALS(helper.getRelativeLoopCount(work_in6,work_in4),1);
    TS_ASSERT_EQUALS(helper.getRelativeLoopCount(work_in4,work_in6),5);
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

#endif /*BINARYOPERATIONTEST_H_*/
