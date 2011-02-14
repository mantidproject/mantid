#ifndef COMMUTATIVEBINARYOPERATIONTEST_H_
#define COMMUTATIVEBINARYOPERATIONTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/CommutativeBinaryOperation.h"

using namespace Mantid;
using namespace Mantid::API;

class CommutativeBinaryOpHelper : public Mantid::Algorithms::CommutativeBinaryOperation
{
public:
  /// Default constructor
  CommutativeBinaryOpHelper() : CommutativeBinaryOperation() {};
  /// Destructor
  virtual ~CommutativeBinaryOpHelper() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "CommutativeBinaryOperationHelper"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  bool checkSizeCompatibility(const MatrixWorkspace_sptr ws1,const MatrixWorkspace_sptr ws2)
  {
    m_lhs = ws1;
    m_rhs = ws2;
    BinaryOperation::checkRequirements();
    return CommutativeBinaryOperation::checkSizeCompatibility(ws1,ws2);
  }

private:
  // Overridden BinaryOperation methods
  void performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                              const MantidVec& rhsY, const MantidVec& rhsE, MantidVec& YOut, MantidVec& EOut)
  {}
  void performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                              const double& rhsY, const double& rhsE, MantidVec& YOut, MantidVec& EOut)
  {}
};

class CommutativeBinaryOperationTest : public CxxTest::TestSuite
{
public:

  void testcheckSizeCompatibility1D1D()
  {
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(10);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(20);
    MatrixWorkspace_sptr work_in3 = WorkspaceCreationHelper::Create1DWorkspaceFib(10);
    MatrixWorkspace_sptr work_in4 = WorkspaceCreationHelper::Create1DWorkspaceFib(5);
    MatrixWorkspace_sptr work_in5 = WorkspaceCreationHelper::Create1DWorkspaceFib(3);
    MatrixWorkspace_sptr work_in6 = WorkspaceCreationHelper::Create1DWorkspaceFib(1);
    CommutativeBinaryOpHelper helper;
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_in2));
    TS_ASSERT(helper.checkSizeCompatibility(work_in1,work_in3));
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_in4));
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_in5));
    TS_ASSERT(helper.checkSizeCompatibility(work_in1,work_in6));
  }

  void testcheckSizeCompatibility2D1D()
  {
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(10,10);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(20);
    MatrixWorkspace_sptr work_in3 = WorkspaceCreationHelper::Create1DWorkspaceFib(10);
    MatrixWorkspace_sptr work_in4 = WorkspaceCreationHelper::Create1DWorkspaceFib(5);
    MatrixWorkspace_sptr work_in5 = WorkspaceCreationHelper::Create1DWorkspaceFib(3);
    MatrixWorkspace_sptr work_in6 = WorkspaceCreationHelper::Create1DWorkspaceFib(1);
    MatrixWorkspace_sptr work_event1 = WorkspaceCreationHelper::CreateEventWorkspace(10,1);
    MatrixWorkspace_sptr work_event2 = WorkspaceCreationHelper::CreateEventWorkspace(1,10);
    CommutativeBinaryOpHelper helper;
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_in2));
    TS_ASSERT(helper.checkSizeCompatibility(work_in1,work_in3));
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_in4));
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_in5));
    TS_ASSERT(helper.checkSizeCompatibility(work_in1,work_in6));
    TS_ASSERT(helper.checkSizeCompatibility(work_in1,work_event1));
    //bin boundaries will not match
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_event2));
  }


  void testcheckSizeCompatibility2D2D()
  {
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(10,10);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(10,20);
    MatrixWorkspace_sptr work_in3 = WorkspaceCreationHelper::Create2DWorkspace(10,10);
    MatrixWorkspace_sptr work_in4 = WorkspaceCreationHelper::Create2DWorkspace(5,5);
    MatrixWorkspace_sptr work_in5 = WorkspaceCreationHelper::Create2DWorkspace(3,3);
    MatrixWorkspace_sptr work_in6 = WorkspaceCreationHelper::Create2DWorkspace(100,1);
    MatrixWorkspace_sptr work_in7 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(10.0);
    MatrixWorkspace_sptr work_event1 = WorkspaceCreationHelper::CreateEventWorkspace(10,1);
    MatrixWorkspace_sptr work_event2 = WorkspaceCreationHelper::CreateEventWorkspace(10,10);
    CommutativeBinaryOpHelper helper;
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_in2));
    TS_ASSERT(helper.checkSizeCompatibility(work_in1,work_in3));
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_in4));
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_in5));
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_in6));
    TS_ASSERT(helper.checkSizeCompatibility(work_in1,work_in7));
    TS_ASSERT(helper.checkSizeCompatibility(work_in7,work_in1));
    TS_ASSERT(helper.checkSizeCompatibility(work_in1,work_event1));
    TS_ASSERT(helper.checkSizeCompatibility(work_in1,work_event2));
  }

  void checkOutputWorkspace(MatrixWorkspace_sptr ws, MatrixWorkspace_sptr wsIn1,MatrixWorkspace_sptr wsIn2 ) const
  {
    int targetsize = (wsIn1->size()>wsIn2->size())?wsIn1->size():wsIn2->size();
    TS_ASSERT_EQUALS(ws->size(),targetsize);
    //check they are all 0
    for(MatrixWorkspace::iterator ti(*ws); ti != ti.end(); ++ti)
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
