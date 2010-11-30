#ifndef MULTIPLYTEST_H_
#define MULTIPLYTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "WorkspaceCreationHelper.hh"
#include "MantidAlgorithms/Divide.h"
#include "MantidAlgorithms/Multiply.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceOpOverloads.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;


/*****************************************************************************************/
/********** PLEASE NOTE! THIS TEST IS SHARED (copy/pasted) WITH DivideTest.h *************/
/*****************************************************************************************/
class MultiplyTest : public CxxTest::TestSuite
{
public:
  bool DO_DIVIDE;

  MultiplyTest()
  {
    DO_DIVIDE = false;
  }

  void testInit()
  {
    Divide alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    //Setting properties to input workspaces that don't exist throws
    TS_ASSERT_THROWS( alg.setPropertyValue("LHSWorkspace","test_in21"), std::invalid_argument );
    TS_ASSERT_THROWS( alg.setPropertyValue("RHSWorkspace","test_in22"), std::invalid_argument );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace","test_out2") );
  }


  void testDivideWithMaskedSpectraProducesZeroes()
  {
    doDivideWithMaskedTest(false);
  }

  void testDivideWithMaskedSpectraProducesZeroesWhenReplacingInputWorkspace()
  {
    doDivideWithMaskedTest(true);
  }

  void testCompoundAssignment()
  {
    MatrixWorkspace_sptr a = WorkspaceCreationHelper::CreateWorkspaceSingleValue(3);
    const Workspace_const_sptr b = a;
    MatrixWorkspace_sptr c = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2);
    a /= 5;
    TS_ASSERT_EQUALS(a->readY(0)[0],0.6);
    TS_ASSERT_EQUALS(a,b);
    a /= c;
    TS_ASSERT_EQUALS(a->readY(0)[0],0.3);
    TS_ASSERT_EQUALS(a,b);
  }


  //========================================= 2D and 1D Workspaces ==================================

  void testExec_1D_1D()
  {
    int sizex = 10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    performTest(work_in1,work_in2);
  }

  void testExec_2D_2D()
  {
    int sizex = 10,sizey=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(sizex,sizey);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);
    performTest(work_in1,work_in2);
  }

  void testExec_2D_1D()
  {
    int sizex = 10,sizey=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    performTest(work_in1,work_in2);
  }

  void testExec_1D_Rand2D()
  {
    int sizex = 10,sizey=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceRand(sizex);
    performTest(work_in1,work_in2);
  }

  void testExec_2D_1DVertical()
  {
    int sizex = 10,sizey=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace123(1,sizey);
    performTest(work_in1,work_in2);
  }

  void testExec_2D_2DSingleSpectrumBiggerSize_fails()
  {
    //In 2D workspaces, the X bins have to match
    int sizex = 10,sizey=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(sizex,sizey);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(1,sizey*2);
    performTest_fails(work_in1, work_in2);
  }

  void testExec_2D_2DbyOperatorOverload()
  {
    int sizex = 10,sizey=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(sizex,sizey);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);
    MatrixWorkspace_sptr work_out1;
    if (DO_DIVIDE)
      work_out1 = work_in1/work_in2;
    else
      work_out1 = work_in1*work_in2;

    checkData(work_in1, work_in2, work_out1);
  }

  void testExec_1D_SingleValue()
  {
    int sizex = 10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.2);
    performTest(work_in1,work_in2);
  }

  void testExec_SingleValue_1D_fails()
  {
    int sizex = 10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    performTest_fails(work_in1,work_in2);
  }

  void testExec_2D_SingleValue()
  {
    int sizex = 5,sizey=300;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspaceBinned(sizex,sizey);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(4.455);
    performTest(work_in1,work_in2);
  }

  void testExec_SingleValue_2D_fails()
  {
    int sizex = 5,sizey=300;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(4.455);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspaceBinned(sizex,sizey);
    performTest_fails(work_in1,work_in2);
  }

  void testExec_2D_SingleValueNoError()
  {
    int sizex = 5,sizey=300;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspaceBinned(sizex,sizey);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValueWithError(5.0, 0.0);
    performTest(work_in1,work_in2);
  }




  //========================================= EventWorkspaces ==================================

  // Equivalent of 2D over 2D, really
  void testExec_2D_Event()
  {
    int sizex = 10,sizey=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(sizex,sizey);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(sizex,sizey,100,0.0,1.0,2);
    performTest(work_in1,work_in2, false);
  }

  void testExec_Event_2D()
  {
    int sizex = 10,sizey=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(sizex,sizey,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(sizex,sizey);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75));
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12));
  }

  void testExec_Event_2DSingleSpectrum()
  {
    int sizex = 10,sizey=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(sizex,sizey,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(1,sizey);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75));
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12));
  }

  void testExec_Event_2DSingleSpectrumBiggerSize()
  {
    //Unlike 2D workspaces, you can divide by a single spectrum with different X bins!
    int sizex = 10,sizey=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(sizex,sizey,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(1,sizey*2);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75));
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12));
  }

  void testExec_Event_SingleValue()
  {
    int sizex = 10,sizey=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(sizex,sizey,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.0);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75));
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12));
  }

  void testExec_Event_SingleValueNoError()
  {
    int sizex = 10,sizey=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(sizex,sizey,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValueWithError(2.0, 0.0);
    performTest(work_in1,work_in2, true);
  }

  void testExec_Event_Event()
  {
    int sizex = 20,sizey=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(sizex,sizey,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(sizex,sizey,100,0.0,1.0,2);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75));
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12));
  }

  void testExec_SingleValue_Event_fails()
  {
    int sizex = 10,sizey=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.0);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(sizex,sizey,100,0.0,1.0,2);
    performTest_fails(work_in1,work_in2);
  }

//  void testExec_Event_1D()
//  {
//    int sizex = 20,sizey=10;
//    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(sizex,sizey,100,0.0,1.0,2);
//    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceConstant(sizex, 2.0, sqrt(2.0));
//    performTest(work_in1, work_in2, true,  1.0, sqrt(0.75));
//  }



private:

  /** Divide work_in1 by work_in2.
   * If outputIsEvent is true, check that the ouput is a EventWorkspace.
   * If expectedValue and expectedError are specified, look for all data items to be those values.
   */
  MatrixWorkspace_sptr performTest(const MatrixWorkspace_sptr work_in1, const MatrixWorkspace_sptr work_in2
      , bool outputIsEvent = false, double expectedValue=-1.0, double expectedError=-1.0)
  {

    IAlgorithm * alg;
    if (DO_DIVIDE)
      alg = new Divide();
    else
      alg = new Multiply();

    std::string wsName1 = "DivideTest_test_in1";
    std::string wsName2 = "DivideTest_test_in2";
    std::string wsNameOut = "DivideTest_test_out";
    AnalysisDataService::Instance().add(wsName1, work_in1);
    AnalysisDataService::Instance().add(wsName2, work_in2);
    alg->initialize();
    alg->setPropertyValue("LHSWorkspace",wsName1);
    alg->setPropertyValue("RHSWorkspace",wsName2);
    alg->setPropertyValue("OutputWorkspace",wsNameOut);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT( alg->isExecuted() );
    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsNameOut)));

    if (work_out1)
    {
      //Check that the output is an event workspace?
      if (outputIsEvent)
      {
        TS_ASSERT( boost::dynamic_pointer_cast<EventWorkspace>(work_out1) );
      }

      checkData(work_in1, work_in2, work_out1, 0, expectedValue, expectedError);

      AnalysisDataService::Instance().remove(wsNameOut);
    }

    AnalysisDataService::Instance().remove(wsName1);
    AnalysisDataService::Instance().remove(wsName2);
    delete alg;
    return work_out1;
  }


  /** Perform the algorithm, check that if fails! */
  void performTest_fails(const MatrixWorkspace_sptr work_in1, const MatrixWorkspace_sptr work_in2)
  {
    IAlgorithm * alg;
    if (DO_DIVIDE)
      alg = new Divide();
    else
      alg = new Multiply();

    std::string wsName1 = "DivideTest_test_in1";
    std::string wsName2 = "DivideTest_test_in2";
    std::string wsNameOut = "DivideTest_test_out";
    AnalysisDataService::Instance().add(wsName1, work_in1);
    AnalysisDataService::Instance().add(wsName2, work_in2);
    alg->initialize();
    alg->setPropertyValue("LHSWorkspace",wsName1);
    alg->setPropertyValue("RHSWorkspace",wsName2);
    alg->setPropertyValue("OutputWorkspace",wsNameOut);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT( !alg->isExecuted() );

    AnalysisDataService::Instance().remove(wsName1);
    AnalysisDataService::Instance().remove(wsName2);
    AnalysisDataService::Instance().remove(wsNameOut);
    delete alg;
  }


  void checkData( MatrixWorkspace_sptr work_in1,  MatrixWorkspace_sptr work_in2, MatrixWorkspace_sptr work_out1)
  {
    //default to a horizontal loop orientation
    checkData(work_in1,work_in2,work_out1,0);
  }

  // loopOrientation 0=Horizontal, 1=Vertical
  void checkData( const MatrixWorkspace_sptr work_in1,  const MatrixWorkspace_sptr work_in2, const MatrixWorkspace_sptr work_out1,
      int loopOrientation, double expectedValue=-1.0, double expectedError=-1.0)
  {
    int ws2LoopCount;
    if (work_in2->size() > 0)
    {
      ws2LoopCount = work_in1->size()/work_in2->size();
    }
    ws2LoopCount = (ws2LoopCount==0) ? 1 : ws2LoopCount;

    for (int i = 0; i < work_out1->size(); i++)
    {
      int ws2Index = i;

      if (ws2LoopCount > 1)
      {
        if (loopOrientation == 0)
        {
          ws2Index = i%ws2LoopCount;
        }
        else
        {
          ws2Index = i/ws2LoopCount;
        }
      }
      if (!checkDataItem(work_in1,work_in2,work_out1,i,ws2Index, expectedValue, expectedError))
        break;
    }
  }

  bool checkDataItem (const MatrixWorkspace_sptr work_in1,  const MatrixWorkspace_sptr work_in2, const MatrixWorkspace_sptr work_out1,
      int i, int ws2Index,
      double expectedValue=-1.0, double expectedError=-1.0
      )
  {
      double sig1 = work_in1->readY(i/work_in1->blocksize())[i%work_in1->blocksize()];
      double sig2 = work_in2->readY(ws2Index/work_in2->blocksize())[ws2Index%work_in2->blocksize()];
      double sig3 = work_out1->readY(i/work_in1->blocksize())[i%work_in1->blocksize()];
      TS_ASSERT_DELTA(work_in1->readX(i/work_in1->blocksize())[i%work_in1->blocksize()],
        work_out1->readX(i/work_in1->blocksize())[i%work_in1->blocksize()], 0.0001);

      double err1 = work_in1->readE(i/work_in1->blocksize())[i%work_in1->blocksize()];
      double err2 = work_in2->readE(ws2Index/work_in2->blocksize())[ws2Index%work_in2->blocksize()];
      double err3 = work_out1->readE(i/work_in1->blocksize())[i%work_in1->blocksize()];

      double value, error;
      if (expectedValue==-1.0 && expectedError==-1.0)
      {
        //Compute the expectation
        if (DO_DIVIDE)
          value = sig1 / sig2;
        else
          value = sig1 * sig2;

        error = sig3 * sqrt(((err1/sig1)*(err1/sig1)) + ((err2/sig2)*(err2/sig2)));;
      }
      else
      {
        //Use the parameters
        error = expectedError;
        value = expectedValue;
      }

      double diff = err3 - error;
      if (diff < 0) diff = -diff;

      TS_ASSERT_DELTA(value, sig3, 0.0001);
      TS_ASSERT_DELTA(error, err3, 0.0001);

      // Return false if the error is wrong
      return (diff < 0.0001);
  }

  void doDivideWithMaskedTest(bool replaceInput)
  {
    const int sizex = 10,sizey=20;
    std::set<int> masking;
    masking.insert(0);
    masking.insert(2);
    masking.insert(7);

    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(sizex,sizey, 0, masking);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey, 0, masking);
    const std::string lhs("work_in1"), rhs("work_in2");
    AnalysisDataService::Instance().add(lhs, work_in1);
    AnalysisDataService::Instance().add(rhs, work_in2);

    //Zero some data to test mask propagation
    for( int j = 0; j < sizex; ++j )
    {
      work_in1->dataY(0)[j] = 0.0;
      work_in1->dataY(2)[j] = 0.0;
      work_in1->dataY(7)[j] = 0.0;

      work_in2->dataY(0)[j] = 0.0;
      work_in2->dataY(2)[j] = 0.0;
      work_in2->dataY(7)[j] = 0.0;
    }

    Divide helper;
    helper.initialize();
    helper.setPropertyValue("LHSWorkspace", lhs);
    helper.setPropertyValue("RHSWorkspace", rhs);
    std::string outputSpace;
    if( replaceInput )
    {
      outputSpace = lhs;
    }
    else
    {
      outputSpace = "lhsOverRhs";
    }
    helper.setPropertyValue("OutputWorkspace", lhs);
    helper.execute();

    TS_ASSERT(helper.isExecuted());

    MatrixWorkspace_sptr output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("work_in1"));
    TS_ASSERT(output);

    for( int i = 0; i < sizey; ++i )
    {
      IDetector_sptr det = output->getDetector(i);
      TS_ASSERT(det);
      if( !det ) TS_FAIL("No detector found");
      if( masking.count(i) == 0 )
      {
        TS_ASSERT_EQUALS(det->isMasked(), false);
      }
      else
      {
        TS_ASSERT_EQUALS(det->isMasked(), true);
        double yValue = output->readY(i)[0];
        TS_ASSERT_EQUALS(yValue, yValue );
        TS_ASSERT_DIFFERS(yValue, std::numeric_limits<double>::infinity() );
      }
    }
    AnalysisDataService::Instance().remove(lhs);
    AnalysisDataService::Instance().remove(rhs);
    if( !replaceInput ) AnalysisDataService::Instance().remove(outputSpace);
  }


};

#endif /*MULTIPLYTEST_H_*/
