#ifndef MULTIPLYTEST_H_
#define MULTIPLYTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/Divide.h"
#include "MantidAlgorithms/Multiply.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidDataObjects/EventWorkspaceHelpers.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using Mantid::Geometry::IDetector_sptr;

/*****************************************************************************************/
/********** PLEASE NOTE! THIS TEST IS SHARED (copy/pasted) WITH DivideTest.h *************/
/*****************************************************************************************/
class MultiplyTest : public CxxTest::TestSuite
{
public:
  bool DO_DIVIDE;
  std::string message;

  MultiplyTest()
  {
    DO_DIVIDE = false;
  }





  void testInit()
  {
    IAlgorithm * alg = NULL;
    if (DO_DIVIDE)
    {
      alg = new Divide;
    }
    else
    {
      alg = new Multiply;
    }
    TS_ASSERT( alg );
    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    TS_ASSERT(alg->isInitialized());
    //Setting properties to input workspaces that don't exist throws
    TS_ASSERT_THROWS( alg->setPropertyValue("LHSWorkspace","test_in21"), std::invalid_argument );
    TS_ASSERT_THROWS( alg->setPropertyValue("RHSWorkspace","test_in22"), std::invalid_argument );
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace","test_out2") );
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

  void test_1D_1D()
  {
    int nBins = 10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(nBins);
    performTest(work_in1,work_in2);
  }

  void test_2D_2D()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(nHist,nBins);
    performTest(work_in1,work_in2);
  }

  void test_2D_2D_inPlace()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
    performTest(work_in1,work_in2, false /*not event*/,
        DO_DIVIDE ? 1.0 : 4.0, DO_DIVIDE ? 1.0 : 4.0, false, false, true /*in place*/);
  }

  void test_2D_1D()
  {
    int nHist = 20,nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(nBins);
    performTest(work_in1,work_in2);
  }

  void test_2D_1DColumn()
  {
    for (int inplace=0; inplace<2; inplace++)
    {
      int nHist = 10,nBins=20;
      MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
      MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(nHist,1);
      performTest(work_in1,work_in2, false /*not event*/,
          DO_DIVIDE ? 1.0 : 4.0, DO_DIVIDE ? 1.0 : 4.0, false, false, inplace /*in place*/);
    }
  }

  void test_1D_Rand2D()
  {
    int nHist = 10,nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceRand(nBins);
    performTest(work_in1,work_in2);
  }

  void test_2D_1DVertical()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace123(1,nBins);
    performTest(work_in1,work_in2);
  }

  void test_2D_2DSingleSpectrumBiggerSize_fails()
  {
    //In 2D workspaces, the X bins have to match
    int nHist = 20,nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(1,nBins*5);
    performTest_fails(work_in1, work_in2);
  }

  void test_2D_2DbyOperatorOverload()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(nHist,nBins);
    MatrixWorkspace_sptr work_out1;
    if (DO_DIVIDE)
      work_out1 = work_in1/work_in2;
    else
      work_out1 = work_in1*work_in2;

    checkData(work_in1, work_in2, work_out1);
  }

  void test_2D_2DbyOperatorOverload_inPlace()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
    MatrixWorkspace_sptr work_out1;
    if (DO_DIVIDE)
    {
      work_in1 /= work_in2;
      checkData(work_in1, work_in2, work_in1, 0, 1.0, 1.0);
    }
    else
    {
      work_in1 *= work_in2;
      checkData(work_in1, work_in2, work_in1, 0, 4.0, 4.0);
    }

  }

  void test_1D_SingleValue()
  {
    int nBins = 10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.2);
    performTest(work_in1,work_in2);
  }

  void test_SingleValue_1D_failsIfDivide()
  {
    int nBins = 10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(nBins);
    if (DO_DIVIDE)
      performTest_fails(work_in1,work_in2);
    else
      performTest(work_in1,work_in2,false,-1,-1,false,true); // will commute L and R
  }

  void test_2D_SingleValue()
  {
    for (int inplace=0; inplace<2; inplace++)
    {
      int nHist = 10,nBins=20;
      MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
      MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.0);
      performTest(work_in1,work_in2, false /*not event*/,
          DO_DIVIDE ? 1.0 : 4.0, DO_DIVIDE ? 1.0 : 4.0, false, false, inplace /*in place*/);
    }
  }

  void test_SingleValue_2D_failsIfDivide()
  {
    int nHist = 5,nBins=300;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(4.455);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspaceBinned(nHist,nBins);
    if (DO_DIVIDE)
      performTest_fails(work_in1,work_in2);
    else
      performTest(work_in1,work_in2,false,-1,-1,false,true); // will commute L and R
  }

  void test_2D_SingleValueNoError()
  {
    int nHist = 5,nBins=300;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspaceBinned(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValueWithError(5.0, 0.0);
    performTest(work_in1,work_in2);
  }




  //========================================= EventWorkspaces ==================================

  // Equivalent of 2D * or / 2D, really
  void test_2D_Event()
  {
    int nBins = 10,nHist=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist, nBins,100,0.0,1.0,2);
    performTest(work_in1,work_in2, false);
  }

  void test_1DVertical_EventWithOneBin_willCommute()
  {
    int nBins=1,nHist=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist, nBins,100,0.0,1.0,2);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, false /*output is not Event */, 1.0, 1.0, false, false, false /* not in place */);
    else
      performTest(work_in1,work_in2, true /*output is Event */, 4.0, 3.4641, false, true /*will commute*/, false /* not in place */);
  }

  void test_1DVertical_EventWithOneBin_willCommute_inplace()
  {
    int nBins=1,nHist=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist, nBins,100,0.0,1.0,2);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, false /*output is not Event */, 1.0, 1.0, false, false, true /*in place*/);
    else
      performTest(work_in1,work_in2, true /*output is Event */, 4.0, 3.4641, false, false /*will commute*/, true /*in place*/);
  }

  void test_2D_Event_inPlace()
  {
    int nBins=10,nHist=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist, nBins,100,0.0,1.0,2);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, false /*output is not Event */, 1.0, sqrt(1.0), false, false, true);
    else
      performTest(work_in1,work_in2, false /*output is not Event */, 4.0, 4.0, false, false, true);
  }

  void test_2D_Event_RHSEventWorkspaceHasOnebin()
  {
    int nBins = 10,nHist=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist, 1,100,0.0,100.0,2);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, false /*output is not Event */, 1.0, sqrt(1.0), false, false, false);
    else
      performTest(work_in1,work_in2, false /*output is not Event */, 4.0, 4.0, false, false /*no commute*/, false);
  }

  void test_2D_Event_inPlace_RHSEventWorkspaceHasOnebin()
  {
    int nBins = 10,nHist=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist, 1,100,0.0,100.0,2);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, false /*output is not Event */, 1.0, sqrt(1.0), false, false, true);
    else
      performTest(work_in1,work_in2, false /*output is not Event */, 4.0, 4.0, false, false /*no commute*/, true);
  }

  void test_2D_Event_inPlace_RHSEventWorkspaceHasOnebinAndOneSpectrum()
  {
    int nBins = 10,nHist=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(1, 1,100,0.0,100.0,2);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, false /*output is not Event*/, 1.0, sqrt(1.0), false, false, true);
    else
      performTest(work_in1,work_in2, false /*output is not Event*/, 4.0, 4.0, false, false /*no commute*/, true);
  }

  void test_Event_2D_inplace_LHSEventWorkspaceHasOnebin()
  {
    int nBins = 10,nHist=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist, 1, 2, 0.0, 1.0, 2); // Events are at 0.5
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true /*output is Event */, 1.0, 0.8660, false, false, true);
    else
      // MULTIPLY: This commutes because the RHS workspace is bigger; the LHS workspace is treated as single number
      performTest(work_in1,work_in2, false /*output is not Event */, 4.0, 4.0, false, true /* commute */, true);
  }

  void test_Event_2D_inplace_LHSEventWorkspaceHasOnebinAndOneSpectrum()
  {
    int nBins = 10,nHist=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(1, 1, 2, 0.0, 1.0, 2); // Events are at 0.5
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
    if (DO_DIVIDE)
      performTest_fails(work_in1,work_in2); // Incompatible sizes
    else
      // MULTIPLY: This commutes because the RHS workspace is bigger; the LHS workspace is treated as single number
      performTest(work_in1,work_in2, false /*output is not Event */, 4.0, 4.0, false, true /* will commute */, true);
  }

  void test_Event_2D()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75));
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0));
  }

  void test_Event_2D_inPlace()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75), false, false, true);
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0), false, false, true);
  }





  void test_Event_2DSingleSpectrum()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(1, nBins);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75));
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0));
  }

  void test_Event_2DSingleSpectrum_inPlace()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(1, nBins);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75), false, false, true /* in-place */);
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0), false, false, true /* in-place */);
  }

  void test_Event_2DSingleSpectrumBiggerSize()
  {
    //Unlike 2D workspaces, you can divide by a single spectrum with different X bins!
    int nBins = 10,nHist=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nBins,nHist,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(1, nBins*2);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75));
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0));
  }

  void test_2DSingleSpectrum_Event()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(1, nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    if (DO_DIVIDE)
      performTest_fails(work_in1,work_in2); /* Fails for dividing, since you can't commute */
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0), false, true /*will commute */, false);
  }

  void test_2DSingleSpectrum_Event_inPlace()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(1, nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    if (DO_DIVIDE)
      performTest_fails(work_in1,work_in2); /* Fails for dividing, since you can't commute */
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0), false, true /*will commute */, true /* in-place */);
  }

  void test_2DSingleSpectrumBiggerSize_Event()
  {
    //Unlike 2D workspaces, you can divide by a single spectrum with different X bins!
    int nBins = 10,nHist=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(1, nHist*2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nBins,nHist,100,0.0,1.0,2);
    if (DO_DIVIDE)
      performTest_fails(work_in1,work_in2); /* Fails for dividing, since you can't commute */
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0), false, true /*will commute */, false);
  }

  void test_2DSingleSpectrumBiggerSize_Event_inPlace()
  {
    //Unlike 2D workspaces, you can divide by a single spectrum with different X bins!
    int nBins = 10,nHist=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(1, nBins*2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nBins,nHist,100,0.0,1.0,2);
    if (DO_DIVIDE)
      performTest_fails(work_in1,work_in2); /* Fails for dividing, since you can't commute */
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0), false, true /*will commute */, false);
  }

  void test_Event_SingleValue()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.0);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75));
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0));
  }

  void test_Event_SingleValue_inPlace()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.0);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75), false, false, true /* in-place */);
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0), false, false, true /* in-place */);
  }

  void test_SingleValue_Event_failsForDivide()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.0);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    if (DO_DIVIDE)
      performTest_fails(work_in1,work_in2);
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0), false, true /* will commute */);
  }

  void test_SingleValue_Event_inPlace_failsForDivide()
  {
    // Doing in-place on a single value is silly since it just gets overwritten, but it works!
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.0);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    if (DO_DIVIDE)
      performTest_fails(work_in1,work_in2);
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0), false, true /* will commute */, true);
  }

  void test_Event_SingleValueNoError()
  {
    int nHist = 10,nBins=30;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValueWithError(2.0, 0.0);
    performTest(work_in1,work_in2, true);
  }

  void test_Event_Event()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75));
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0));
  }

  void test_Event_Event_inPlace()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75), false, false, true /* in-place */);
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0), false, false, true /* in-place */);
  }



  //========================================= Grouped EventWorkspaces ==================================

  void doGroupedTest(
          int lhs_grouping, bool lhs2D,
          int rhs_grouping, bool rhs2D,
          double divideValue, double divideError,
          double multiplyValue, double multiplyError)
  {
    std::ostringstream mess;
    mess << "LHS: grouping=" << lhs_grouping << ", 2D=" << lhs2D;
    mess << "; RHS: grouping=" << rhs_grouping << ", 2D=" << rhs2D;
    message = mess.str();

    int numpix = 20;
    std::vector< std::vector<int> > lhs(numpix/lhs_grouping), rhs(numpix/rhs_grouping);
    for (int i=0; i<numpix; i++)
    {
      // lhs_grouping detectors in each on the lhs
      lhs[i/lhs_grouping].push_back(i);
      // rhs_grouping detectors in each on the lhs
      rhs[i/rhs_grouping].push_back(i);
    }
    // Grouped workspace will have lhs_grouping events in each bin (also).
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateGroupedEventWorkspace(lhs, 100, 1.0);
    if (lhs2D)
      work_in1 = EventWorkspaceHelpers::convertEventTo2D(work_in1);
    TS_ASSERT_DELTA( work_in1->readE(0)[0], sqrt( double(lhs_grouping*1.0) ), 1e-5);

    // Grouped workspace will have rhs_grouping events in each bin (also).
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateGroupedEventWorkspace(rhs, 100, 1.0);
    if (rhs2D)
      work_in2 = EventWorkspaceHelpers::convertEventTo2D(work_in2);
    TS_ASSERT_DELTA( work_in2->readE(0)[0], sqrt( double(rhs_grouping*1.0) ), 1e-5);

    if (DO_DIVIDE)
      performTest(work_in1,work_in2, !lhs2D, divideValue, divideError, true);
    else
    {
      bool willCommute = (work_in1->getNumberHistograms() < work_in2->getNumberHistograms()) && !DO_DIVIDE;
      bool willbeEvent = !lhs2D;
      if (willCommute) willbeEvent = !rhs2D;
      performTest(work_in1,work_in2, willbeEvent, multiplyValue, multiplyError, true, willCommute);

    }
    message = "";
  }




  // -------------------------------------------------------------------------------
  void test_NotGrouped_Grouped()
  {
    // Try all 4 cases: event or 2D
    for (int lhs_2D=0; lhs_2D<2; lhs_2D++)
      for (int rhs_2D=0; rhs_2D<2; rhs_2D++)
        // Because there is only 1 event in LHS, the 2D and Event versions are equivalent
        doGroupedTest(1, lhs_2D, 2, rhs_2D, 0.5, sqrt(0.375),   2.0, sqrt(6.0));
  }


  void test_Grouped_Grouped()
  {
    for (int lhs_2D=0; lhs_2D<2; lhs_2D++)
      for (int rhs_2D=0; rhs_2D<2; rhs_2D++)
        if (lhs_2D)
          doGroupedTest(2, lhs_2D, 4, rhs_2D, 0.5, 0.4330,    8.0, sqrt(48.0));
        else
          // Errors are different when LHS is events!
          doGroupedTest(2, lhs_2D, 4, rhs_2D, 0.5, 0.3952,    8.0, sqrt(40.0));
  }

  void test_Grouped_NotGrouped()
  {
    for (int lhs_2D=0; lhs_2D<2; lhs_2D++)
      for (int rhs_2D=0; rhs_2D<2; rhs_2D++)
        if (DO_DIVIDE)
        {
          // Ends up empty because you can't place RHS into LHS
          //doGroupedTest(2, lhs_2D, 1, rhs_2D, 0.0, 0.0, 0.0, 0.0);
        }
        else
        {
          if (lhs_2D)
            doGroupedTest(2, lhs_2D, 1, rhs_2D, 2.0, sqrt(2.0),    2.0, sqrt(6.0));
          else
            doGroupedTest(2, lhs_2D, 1, rhs_2D, 2.0, sqrt(2.0),    2.0, sqrt(6.0));
        }
  }

  void test_GroupedEvent_NotGrouped2D()
  {
    doGroupedTest(2, false, 1, true, 2.0, sqrt(2.0),    2.0, sqrt(6.0));
  }


  std::string describe_workspace(const MatrixWorkspace_sptr ws)
  {
    std::ostringstream mess;
    EventWorkspace_const_sptr ews = boost::dynamic_pointer_cast<const EventWorkspace>(ws);
    if (ews)
      mess << "Event";
    else
      mess << "2D";
    mess << "(" << ws->getNumberHistograms() << " spectra," << ws->blocksize() << " bins,";
    mess << "Y[0][0] = " << ws->readY(0)[0] << ")";
    return mess.str();
  }

  /** Divide/Multiply work_in1 by work_in2.
   * If outputIsEvent is true, check that the ouput is a EventWorkspace.
   * If expectedValue and expectedError are specified, look for all data items to be those values.
   *
   * @param algorithmWillCommute :: the algorithm will swap LHS and RHS when calculating.
   *        Take that into accound when calculating the expected result.
   *
   */
  MatrixWorkspace_sptr performTest(const MatrixWorkspace_sptr work_in1, const MatrixWorkspace_sptr work_in2,
      bool outputIsEvent = false, double expectedValue=-1.0, double expectedError=-1.0,
      bool allowMismatchedSpectra = false, bool algorithmWillCommute = false,
      bool doInPlace = false)
  {
    bool automessage = false;
    if (message == "")
    {
      automessage = true;
      // Build up the descriptive message
      std::ostringstream mess;
      mess << "WITH: ";
      mess << describe_workspace(work_in1);
      if (DO_DIVIDE)
        mess << " divided by ";
      else
        mess << " multiplied by ";
      mess << describe_workspace(work_in2);
      if (doInPlace)
        mess << " done in place";
      message = mess.str();
    }

    IAlgorithm * alg;
    if (DO_DIVIDE)
      alg = new Divide();
    else
      alg = new Multiply();

    std::string base = DO_DIVIDE ? "DivideTest_" : "MultiplyTest";
    std::string wsName1 = base + "_in1";
    std::string wsName2 = base + "_in2";

    // Make the output workspace name; but will be the same as input if doing it in place.
    std::string wsNameOut = base + "_out";
    if (doInPlace)
    {
      wsNameOut = wsName1;
      if (algorithmWillCommute) wsNameOut = wsName2;

    }
    AnalysisDataService::Instance().add(wsName1, work_in1);
    AnalysisDataService::Instance().add(wsName2, work_in2);
    alg->initialize();
    alg->setPropertyValue("LHSWorkspace",wsName1);
    alg->setPropertyValue("RHSWorkspace",wsName2);
    alg->setPropertyValue("OutputWorkspace",wsNameOut);
    alg->setProperty("AllowDifferentNumberSpectra", allowMismatchedSpectra);
    TSM_ASSERT_THROWS_NOTHING(message, alg->execute());
    TSM_ASSERT( message, alg->isExecuted() );
    MatrixWorkspace_sptr work_out1;
    TSM_ASSERT_THROWS_NOTHING(message, work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsNameOut)));
    TSM_ASSERT( message, work_out1 );
    if (work_out1)
    {
      //Check that the output is an event workspace?
      if (outputIsEvent)
      {
        TSM_ASSERT( message, boost::dynamic_pointer_cast<EventWorkspace>(work_out1) );
      }
      else
      {
        // Check that it is NOT event
        TSM_ASSERT( message, !(boost::dynamic_pointer_cast<EventWorkspace>(work_out1)) );
      }

      if (algorithmWillCommute)
        checkData(work_in2, work_in1, work_out1, 0, expectedValue, expectedError);

      else
        checkData(work_in1, work_in2, work_out1, 0, expectedValue, expectedError);

      AnalysisDataService::Instance().remove(wsNameOut);
    }

    AnalysisDataService::Instance().remove(wsName1);
    AnalysisDataService::Instance().remove(wsName2);
    delete alg;

    // Return to the empty message for next time
    if (automessage) message = "";

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

    std::string base = DO_DIVIDE ? "DivideTest_" : "MultiplyTest";
    std::string wsName1 = base + "_in1";
    std::string wsName2 = base + "_in2";
    std::string wsNameOut = base + "_out";
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
    TSM_ASSERT_LESS_THAN( message, 0, work_out1->getNumberHistograms());
    TSM_ASSERT_LESS_THAN( message, 0, work_out1->blocksize());
    TSM_ASSERT_EQUALS( message, work_in1->getNumberHistograms(), work_out1->getNumberHistograms());

    if (expectedValue == -1.0 && expectedError == -1.0)
    {
      // --- Perform an automatic test ------------
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
        if (!checkDataItem(work_in1,work_in2,work_out1,i,ws2Index))
          break;
      }
    }
    else
    {
      // ------ Use expected answer --------------------
      bool breakOut=false;
      for (int wi=0; wi < work_out1->getNumberHistograms(); wi++)
      {
        for (int i=0; i<work_out1->blocksize(); i++)
        {
          TS_ASSERT_DELTA(work_in1->readX(wi)[i], work_out1->readX(wi)[i], 0.0001);
          double sig3 = work_out1->readY(wi)[i];
          double err3 = work_out1->readE(wi)[i];
          TSM_ASSERT_DELTA(message, sig3, expectedValue, 0.0001);
          TSM_ASSERT_DELTA(message, err3, expectedError, 0.0001);
          if (fabs(err3 - expectedError) > 0.001)
          {
            breakOut=true;
            break;
          }

        }
        if (breakOut) break;
      }

    }
  }

  bool checkDataItem (const MatrixWorkspace_sptr work_in1,  const MatrixWorkspace_sptr work_in2, const MatrixWorkspace_sptr work_out1,
      int i, int ws2Index)
  {
    // Avoid going out of bounds! For some of the grouped ones
//    if (i/work_in1->blocksize() >= work_in1->getNumberHistograms())
//      return true;
//    if (ws2Index/work_in2->blocksize() >= work_in2->getNumberHistograms())
//      return true;
    double sig1 = work_in1->readY(i/work_in1->blocksize())[i%work_in1->blocksize()];
    double sig2 = work_in2->readY(ws2Index/work_in2->blocksize())[ws2Index%work_in2->blocksize()];
    double sig3 = work_out1->readY(i/work_in1->blocksize())[i%work_in1->blocksize()];

    TS_ASSERT_DELTA(work_in1->readX(i/work_in1->blocksize())[i%work_in1->blocksize()],
        work_out1->readX(i/work_in1->blocksize())[i%work_in1->blocksize()], 0.0001);

    double err1 = work_in1->readE(i/work_in1->blocksize())[i%work_in1->blocksize()];
    double err2 = work_in2->readE(ws2Index/work_in2->blocksize())[ws2Index%work_in2->blocksize()];
    double err3 = work_out1->readE(i/work_in1->blocksize())[i%work_in1->blocksize()];

    double expectValue, expectError;
    //Compute the expectation
    if (DO_DIVIDE)
      expectValue = sig1 / sig2;
    else
      expectValue = sig1 * sig2;

    expectError = sig3 * sqrt(((err1/sig1)*(err1/sig1)) + ((err2/sig2)*(err2/sig2)));;

    double diff = err3 - expectError;
    if (diff < 0) diff = -diff;

    TSM_ASSERT_DELTA(message, sig3, expectValue, 0.0001);
    TSM_ASSERT_DELTA(message, err3, expectError, 0.0001);

    // Return false if the error is wrong
    return (diff < 0.0001);
  }




  void doDivideWithMaskedTest(bool replaceInput)
  {
    const int nHist = 10,nBins=20;
    std::set<int> masking;
    masking.insert(0);
    masking.insert(2);
    masking.insert(7);

    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(nHist,nBins, 0, masking);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(nHist,nBins, 0, masking);
    const std::string lhs("work_in1"), rhs("work_in2");
    AnalysisDataService::Instance().add(lhs, work_in1);
    AnalysisDataService::Instance().add(rhs, work_in2);

    //Zero some data to test mask propagation
    for( int j = 0; j < nHist; ++j )
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

    for( int i = 0; i < nHist; ++i )
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

#endif /*MULTIPLYTEST_H_ or DIVIDETEST_H*/
