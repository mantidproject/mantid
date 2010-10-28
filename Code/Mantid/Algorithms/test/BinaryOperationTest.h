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

using namespace Mantid;
using namespace Mantid::API;

class BinaryOpHelper : public Mantid::Algorithms::BinaryOperation
{
public:
  /// Default constructor
  BinaryOpHelper() : BinaryOperation() {};
  /// Destructor
  virtual ~BinaryOpHelper() {};

  /// function to return a name of the algorithm, must be overridden in all algorithms
  virtual const std::string name() const { return "BinaryOpHelper"; }
  /// function to return a version of the algorithm, must be overridden in all algorithms
  virtual int version() const { return 1; }
  /// function to return a category of the algorithm. A default implementation is provided
  virtual const std::string category() const {return "Helper";}
  
  bool checkSizeCompatibility(const MatrixWorkspace_sptr ws1,const MatrixWorkspace_sptr ws2)
  {
    m_lhs = ws1;
    m_rhs = ws2;
    BinaryOperation::checkRequirements();
    return BinaryOperation::checkSizeCompatibility(ws1,ws2);
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

class BinaryOperationTest : public CxxTest::TestSuite
{
public:

  void testcheckSizeCompatibility1D1D()
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
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_in2));
    TS_ASSERT(helper.checkSizeCompatibility(work_in1,work_in3));
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_in4));
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_in5));
    TS_ASSERT(helper.checkSizeCompatibility(work_in1,work_in6));
  }

  void testcheckSizeCompatibility2D1D()
  {
    // Register the workspace in the data service
    Workspace2D_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(10,10);
    Workspace1D_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(20);
    Workspace1D_sptr work_in3 = WorkspaceCreationHelper::Create1DWorkspaceFib(10);
    Workspace1D_sptr work_in4 = WorkspaceCreationHelper::Create1DWorkspaceFib(5);
    Workspace1D_sptr work_in5 = WorkspaceCreationHelper::Create1DWorkspaceFib(3);
    Workspace1D_sptr work_in6 = WorkspaceCreationHelper::Create1DWorkspaceFib(1);
    MatrixWorkspace_sptr work_inEvent1 = WorkspaceCreationHelper::CreateEventWorkspace(10,1);
    //will not pass x array does not match
    MatrixWorkspace_sptr work_inEvent2 = WorkspaceCreationHelper::CreateEventWorkspace(1,10);
    BinaryOpHelper helper;
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_in2));
    TS_ASSERT(helper.checkSizeCompatibility(work_in1,work_in3));
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_in4));
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_in5));
    TS_ASSERT(helper.checkSizeCompatibility(work_in1,work_in6));
    TS_ASSERT(helper.checkSizeCompatibility(work_in1,work_inEvent1));
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_inEvent2));
  }

  void testcheckSizeCompatibility2D2D()
  {
    // Register the workspace in the data service
    Workspace2D_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(10,10);
    Workspace2D_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(20,10);
    Workspace2D_sptr work_in3 = WorkspaceCreationHelper::Create2DWorkspace(10,10);
    Workspace2D_sptr work_in4 = WorkspaceCreationHelper::Create2DWorkspace(5,5);
    Workspace2D_sptr work_in5 = WorkspaceCreationHelper::Create2DWorkspace(3,3);
    Workspace2D_sptr work_in6 = WorkspaceCreationHelper::Create2DWorkspace(1,100);
    MatrixWorkspace_sptr work_inEvent1 = WorkspaceCreationHelper::CreateEventWorkspace(5,5);
    MatrixWorkspace_sptr work_inEvent2 = WorkspaceCreationHelper::CreateEventWorkspace(10,10);
    BinaryOpHelper helper;
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_in2));
    TS_ASSERT(helper.checkSizeCompatibility(work_in1,work_in3));
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_in4));
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_in5));
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_in6));
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1,work_inEvent1));
    TS_ASSERT(helper.checkSizeCompatibility(work_in1,work_inEvent2));
  }

  void testMaskedSpectraPropagation()
  {
    const int sizex = 10,sizey=20;
    std::set<int> masking;
    masking.insert(0);
    masking.insert(2);
    masking.insert(4);
    
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(sizex,sizey, 0, masking);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    BinaryOpHelper helper;
    helper.initialize();
    helper.setProperty("LHSWorkspace", work_in1);
    helper.setProperty("RHSWorkspace", work_in2);
    const std::string outputSpace("test");
    helper.setPropertyValue("OutputWorkspace", outputSpace);
    helper.setRethrows(true);
    helper.execute();

    
    TS_ASSERT(helper.isExecuted());

    MatrixWorkspace_sptr output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace));
    TS_ASSERT(output);
    
    for( int i = 0; i < sizey; ++i )
    {
      IDetector_sptr det;
      try
      {
	det = output->getDetector(i);
      }
      catch(Kernel::Exception::NotFoundError&)
      {
      }
      
      TS_ASSERT(det);
      if( !det ) TS_FAIL("No detector found");
      if( masking.count(i) == 0 )
      {
	TS_ASSERT_EQUALS(det->isMasked(), false);
      }
      else
      {
	TS_ASSERT_EQUALS(det->isMasked(), true);
      }
    }
    
  }

};

#endif /*BINARYOPERATIONTEST_H_*/
