#ifndef PLUSTEST_H_
#define PLUSTEST_H_
#include <cxxtest/TestSuite.h>
#include <cmath>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/Minus.h"
#include "MantidAlgorithms/Plus.h"
#include "MantidAlgorithms/Rebin.h"
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
/********** PLEASE NOTE! THIS TEST IS SHARED (copy/pasted) WITH MinusTest.h **************/
/*****************************************************************************************/


class PlusTest : public CxxTest::TestSuite
{
public:

  bool DO_PLUS;
  std::string message;

  PlusTest()
  {
    numBins = 10;
    numPixels = 6;
    wsNameOut = "MinusTest_outputWorkspace";
    DO_PLUS = true;
  }


  void testInit()
  {
    IAlgorithm * alg = NULL;
    if (DO_PLUS)
    {
      alg = new Plus;
    }
    else
    {
      alg = new Minus;
    }
    TS_ASSERT( alg );
    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    TS_ASSERT(alg->isInitialized());
    //Setting properties to input workspaces that don't exist throws
    TS_ASSERT_THROWS( alg->setPropertyValue("LHSWorkspace","test_in21"), std::invalid_argument );
    TS_ASSERT_THROWS( alg->setPropertyValue("RHSWorkspace","test_in22"), std::invalid_argument );
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace","test_out2") );
  }




  //====================================================================================
  //====================================================================================
  //====================================================================================


  void test_CompoundAssignment()
  {
    MatrixWorkspace_sptr a = WorkspaceCreationHelper::CreateWorkspaceSingleValue(3);
    const Workspace_const_sptr b = a;
    MatrixWorkspace_sptr c = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2);
    if (DO_PLUS)
    {
      a += 5;
      TS_ASSERT_EQUALS(a->readY(0)[0],8);
      TS_ASSERT_EQUALS(a,b);
      a += c;
      TS_ASSERT_EQUALS(a->readY(0)[0],10);
      TS_ASSERT_EQUALS(a,b);
    }
    else
    {
      a -= 5;
      TS_ASSERT_EQUALS(a->readY(0)[0],-2);
      TS_ASSERT_EQUALS(a,b);
      a -= c;
      TS_ASSERT_EQUALS(a->readY(0)[0],-4);
      TS_ASSERT_EQUALS(a,b);
    }
  }


  /// The Plus algorithm sums values in the Run object. Minus does not.
  void test_RunAddition()
  {
    if (DO_PLUS)
    {
      MatrixWorkspace_sptr a = WorkspaceCreationHelper::CreateWorkspaceSingleValue(3);
      a->mutableRun().setProtonCharge(10.);
      MatrixWorkspace_sptr b = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2);
      b->mutableRun().setProtonCharge(5.);

      AnalysisDataService::Instance().add("a", a);
      AnalysisDataService::Instance().add("b", b);

      Plus alg;
      alg.initialize();
      TS_ASSERT_THROWS_NOTHING(
          alg.setPropertyValue("LHSWorkspace","a");
      alg.setPropertyValue("RHSWorkspace","b");
      alg.setPropertyValue("OutputWorkspace","c");
      )
      alg.execute();

      MatrixWorkspace_sptr work_out1;
      TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("c")));

      TS_ASSERT_DELTA(work_out1->run().getProtonCharge(), 15.0, 1e-8);

      AnalysisDataService::Instance().remove("a");
      AnalysisDataService::Instance().remove("b");
      AnalysisDataService::Instance().remove("c");
    }
  }




  //====================================================================================
  //====================================================================================
  //====================================================================================


  void test_1D_1D()
  {
    int nBins = 10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(nBins);
    performTest(work_in1,work_in2);
  }
  void test_1D_1DRand()
  {
    int nBins = 10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceRand(nBins);
    performTest(work_in1,work_in2);
  }

  void test_2D_2D_NotHistograms()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(nHist,nBins);
    performTest(work_in1,work_in2);
  }

  void test_2D_2D_Histograms()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(nHist,nBins, true);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(nHist,nBins, true);
    performTest(work_in1,work_in2);
  }

  void test_2D_1D()
  {
    int nHist = 20,nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(nBins);
    performTest(work_in1,work_in2);
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

  void test_1DVertical_2D()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(1,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(nHist,nBins);
    if (DO_PLUS)
      performTest(work_in1,work_in2);
    else
      performTest_fails(work_in1,work_in2);
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
    if (DO_PLUS)
      work_out1 = work_in1+work_in2;
    else
      work_out1 = work_in1-work_in2;

    checkData(work_in1, work_in2, work_out1);
  }

  void test_1D_SingleValue()
  {
    int nBins = 10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.2);
    performTest(work_in1,work_in2);
  }

  void test_SingleValue_1D()
  {
    int nBins = 10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(nBins);
    if (DO_PLUS)
      performTest(work_in1,work_in2);
    else
      performTest_fails(work_in1,work_in2);
  }

  void test_2D_SingleValue()
  {
    int nHist = 5,nBins=300;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspaceBinned(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(4.455);
    performTest(work_in1,work_in2);
  }

  void test_2D_SingleValue_InPlace()
  {
    int nHist =10,nBins=300;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspaceBinned(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(4.455);
    performTest(work_in1,work_in2, true /*in place*/, false /*not event*/,
        DO_PLUS ? 6.455 : -2.455,   2.5406);
  }

  void test_SingleValue_2D()
  {
    int nHist = 5,nBins=300;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(4.455);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspaceBinned(nHist,nBins);
    if (DO_PLUS)
      performTest(work_in1,work_in2);
    else
      performTest_fails(work_in1,work_in2);
  }

  void test_2D_SingleValueNoError()
  {
    int nHist = 5,nBins=300;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspaceBinned(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValueWithError(5.0, 0.0);
    performTest(work_in1,work_in2);
  }

  //============================================================================================
  //========================================= EventWorkspaces ==================================
  //============================================================================================

  void test_Event_SingleValue()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.0);
    // Become a WS2D
    performTest(work_in1, work_in2, false, false /*output is NOT event*/ );
  }

  void test_Event_SingleValue_inPlace_fails()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.0);
    performTest_fails(work_in1, work_in2, true);
  }

  void test_SingleValue_Event()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.0);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    // Become a WS2D
    if (DO_PLUS)
      performTest(work_in1, work_in2, false, false /*output is NOT event*/ );
    else
      performTest_fails(work_in1,work_in2);

  }

  void test_SingleValue_Event_inPlace_fails()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.0);
    // Become a WS2D
    performTest_fails(work_in1, work_in2, true);
  }



  void test_2D_Event()
  {
    int nBins = 10,nHist=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist, nBins,100,0.0,1.0,2);
    performTest(work_in1,work_in2, false);
  }

  void test_2D_Event_inPlace()
  {
    int nBins = 10,nHist=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist, nBins,100,0.0,1.0,2);
    // You have to specify the expected output value because in1 gets changed.
    performTest(work_in1,work_in2, true, false /*not event out*/,
        DO_PLUS ? 4.0 : 0.0,   DO_PLUS ? 2.0 : 2.0);
  }

  void test_Event_2D()
  {
    int nBins = 10,nHist=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist, nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
    performTest(work_in1,work_in2, false);
  }

  void test_Event_2D_inPlace_fails()
  {
    int nBins = 10,nHist=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist, nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(nHist,nBins);
    performTest_fails(work_in1,work_in2, true);
  }



  void test_Event_2DSingleSpectrum()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(1, nBins);
    performTest(work_in1,work_in2, false);
  }

  void test_Event_2DSingleSpectrum_inPlace_fails()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace(1, nBins);
    performTest_fails(work_in1,work_in2, true);
  }

  void test_2DSingleSpectrum_Event_fails()
  {
    for(int inplace=0; inplace<2;inplace++)
    {
      int nHist = 10,nBins=20;
      MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace(1, nBins);
      MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
      if (DO_PLUS)
        performTest(work_in1,work_in2, inplace, false /*not event*/, 4.0, 2.0); // Commutes if doing it with event workspace
      else
        performTest_fails(work_in1,work_in2, inplace);
    }
  }




  void test_Event_Event()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_out = performTest(work_in1,work_in2, false, true /*outputIsEvent*/);
  }

  void test_Event_Event_inPlace()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_out = performTest(work_in1,work_in2, true, true /*outputIsEvent*/,
        DO_PLUS ? 4.0 : 0.0,   DO_PLUS ? 2.0 : 2.0);
  }

  void test_Event_EventSingleSpectrum_fails()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(1,nBins,100,0.0,1.0,2);
    performTest_fails(work_in1,work_in2, false);
  }

  void test_EventSingleSpectrum_Event_fails()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(1,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    performTest_fails(work_in1,work_in2, false);
  }


  void test_EventWithASingleBin_EventWithASingleBin()
  {
    for(int inplace=0; inplace<2;inplace++)
    {
      int nHist = 10,nBins=1;
      MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
      MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
      MatrixWorkspace_sptr work_out = performTest(work_in1,work_in2, inplace, true /*outputIsEvent*/,
          DO_PLUS ? 4.0 : 0.0,   DO_PLUS ? 2.0 : 2.0);
    }
  }

  void test_Event_EventWithASingleBin()
  {
    for(int inplace=0; inplace<2;inplace++)
    {
      int nHist = 10,nBins=20;
      MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
      MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,1,100,0.0,1.0,2);
      MatrixWorkspace_sptr work_out = performTest(work_in1,work_in2, inplace, true /*outputIsEvent*/,
          DO_PLUS ? 4.0 : 0.0,   DO_PLUS ? 2.0 : 2.0);
    }
  }

  void test_EventWithASingleBin_Event()
  {
    for(int inplace=0; inplace<2;inplace++)
    {
      int nHist = 10,nBins=20;
      MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,1,100,0.0,1.0,2);
      MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
      MatrixWorkspace_sptr work_out = performTest(work_in1,work_in2, inplace, true /*outputIsEvent*/,
          DO_PLUS ? 4.0 : 0.0,   DO_PLUS ? 2.0 : 2.0);
    }
  }

  void test_EventWithASingleBinAndSingleSpectrum_EventWithASingleBinAndSingleSpectrum()
  {
    for(int inplace=0; inplace<2;inplace++)
    {
      int nHist=1,nBins=1;
      MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
      MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
      MatrixWorkspace_sptr work_out = performTest(work_in1,work_in2, inplace, true /*outputIsEvent*/,
          DO_PLUS ? 4.0 : 0.0,   DO_PLUS ? 2.0 : 2.0);
    }
  }


  /** EW1 = EW1 + EW1
   * This would cause an infinite loop.
   */
  void test_Event_InPlace_AllSameWorkspaces()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_out = performTest(work_in1,work_in2, false, true /*outputIsEvent*/,
        DO_PLUS ? 4.0 : 0.0,   DO_PLUS ? 2.0 : 2.0);
  }






  //====================================================================================
  //====================================================================================
  //====================================================================================


  //------------------------------------------------------------------------------------------------
  void test_Event_IncompatibleUnits_fails()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    work_in2->setYUnit("Microfurlongs per Megafortnights");
    performTest_fails(work_in1,work_in2, false /*not inplace*/);
  }


  //------------------------------------------------------------------------------------------------
  void test_Event_differentOutputAndDifferentPixelIDs()
  {
    for (int inplace =0; inplace < 2; inplace++)
    {
      MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(3,10,100, 0.0, 1.0, 3); // 100 ev
      MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateEventWorkspace(3,10,100, 0.0, 1.0, 2, 100); //200 events per spectrum, but the spectra are at different pixel ids

      //First pixel id of rhs is 100
      IndexToIndexMap *rhs_map = work_in2->getWorkspaceIndexToDetectorIDMap();
      TS_ASSERT_EQUALS( (*rhs_map)[0], 100);

      MatrixWorkspace_sptr work_out = performTest(work_in1,work_in2, inplace, true /*outputIsEvent*/,
          DO_PLUS ? 3.0 : -1.0,   DO_PLUS ? 1.7320 : 1.7320);

      //Ya, its an event workspace
      TS_ASSERT(work_out);
      if (!work_out) return;

      //But two detector IDs in each one
      for (int i=0; i<3; i++)
      {
        std::vector<int> detList = work_out->spectraMap().getDetectors(i);
        TS_ASSERT_EQUALS( detList[0], 0+i );
        if (DO_PLUS)
        {
          TS_ASSERT_EQUALS( detList[1], 100+i );
        }
      }
    }
  }







  //============================================================================


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

  bool set_message(const MatrixWorkspace_sptr work_in1, const MatrixWorkspace_sptr work_in2, bool doInPlace)
  {
    if (message == "")
    {
      // Build up the descriptive message
      std::ostringstream mess;
      mess << "WITH: ";
      mess << describe_workspace(work_in1);
      if (DO_PLUS)
        mess << " plus ";
      else
        mess << " minus ";
      mess << describe_workspace(work_in2);
      if (doInPlace)
        mess << " done in place";
      message = mess.str();
      return true;
    }
    else
      return false;
  }

  /** Divide/Multiply work_in1 by work_in2.
   * If outputIsEvent is true, check that the ouput is a EventWorkspace.
   * If expectedValue and expectedError are specified, look for all data items to be those values.
   *
   * @param work_in1
   * @param work_in2
   * @param doInPlace :: do A = A + B
   * @param outputIsEvent :: output workspace will be EventWorkspace
   * @param expectedValue
   * @param expectedError
   * @param allWorkspacesSameName :: do A = A + A
   * @return
   */
  MatrixWorkspace_sptr performTest(const MatrixWorkspace_sptr work_in1, const MatrixWorkspace_sptr work_in2, bool doInPlace = false,
      bool outputIsEvent = false, double expectedValue=-1.0, double expectedError=-1.0,
      bool allWorkspacesSameName = false, bool algorithmWillCommute = false,
      bool allowMismatchedSpectra = false
  )
  {
    bool automessage = set_message(work_in1, work_in2, doInPlace);

    IAlgorithm * alg;
    if (DO_PLUS)
      alg = new Plus();
    else
      alg = new Minus();

    // ------ Original number of events ----------------
    size_t numEvents1=0;
    size_t numEvents2=0;
    EventWorkspace_const_sptr ews_1 = boost::dynamic_pointer_cast<const EventWorkspace>(work_in1) ;
    if (ews_1) numEvents1 = ews_1->getNumberEvents();
    EventWorkspace_const_sptr ews_2 = boost::dynamic_pointer_cast<const EventWorkspace>(work_in2) ;
    if (ews_2) numEvents2 = ews_2->getNumberEvents();

    std::string base = DO_PLUS ? "PlusTest_" : "MinusTest";
    std::string wsName1 = base + "_in1";
    std::string wsName2 = base + "_in2";

    // Make the output workspace name; but will be the same as input if doing it in place.
    std::string wsNameOut = base + "_out";
    if (doInPlace)
    {
      wsNameOut = wsName1;
      if (algorithmWillCommute) wsNameOut = wsName2;
    }

    if (allWorkspacesSameName)
    {
      wsName1 = base + "_inplace3";
      wsName2 = base + "_inplace3";
      wsNameOut = base + "_inplace3";
      AnalysisDataService::Instance().add(wsName1, work_in1);
    }
    else
    {
      AnalysisDataService::Instance().add(wsName1, work_in1);
      AnalysisDataService::Instance().add(wsName2, work_in2);
    }

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
        EventWorkspace_sptr ews_out = boost::dynamic_pointer_cast<EventWorkspace>(work_out1) ;
        TSM_ASSERT( message, ews_out);
        // The # of events is equal to the sum of the original amount
        TSM_ASSERT_EQUALS( message, ews_out->getNumberEvents(), numEvents1 + numEvents2 );
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
  void performTest_fails(const MatrixWorkspace_sptr work_in1, const MatrixWorkspace_sptr work_in2,
      bool doInPlace = false)
  {
    bool automessage = set_message(work_in1, work_in2, doInPlace);

    IAlgorithm * alg;
    if (DO_PLUS)
      alg = new Plus();
    else
      alg = new Minus();

    std::string base = DO_PLUS ? "PlusTest_" : "MinusTest";
    std::string wsName1 = base + "_in1";
    std::string wsName2 = base + "_in2";
    std::string wsNameOut = base + "_out";
    if (doInPlace) wsNameOut = wsName1;
    AnalysisDataService::Instance().add(wsName1, work_in1);
    AnalysisDataService::Instance().add(wsName2, work_in2);
    alg->initialize();
    alg->setPropertyValue("LHSWorkspace",wsName1);
    alg->setPropertyValue("RHSWorkspace",wsName2);
    alg->setPropertyValue("OutputWorkspace",wsNameOut);
    TSM_ASSERT_THROWS_NOTHING( message, alg->execute());
    TSM_ASSERT( message, !alg->isExecuted() );

    AnalysisDataService::Instance().remove(wsName1);
    AnalysisDataService::Instance().remove(wsName2);
    AnalysisDataService::Instance().remove(wsNameOut);
    delete alg;

    // Return to the empty message for next time
    if (automessage) message = "";

  }



  void checkData( MatrixWorkspace_sptr work_in1,  MatrixWorkspace_sptr work_in2, MatrixWorkspace_sptr work_out1)
  {
    //default to a horizontal loop orientation
    checkData(work_in1,work_in2,work_out1,0);
  }

  // loopOrientation 0=Horizontal, 1=Vertical
  void checkData( MatrixWorkspace_sptr work_in1,  MatrixWorkspace_sptr work_in2, MatrixWorkspace_sptr work_out1,
      int loopOrientation, double expectedValue=-1.0, double expectedError=-1.0  )
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
    if (DO_PLUS)
      expectValue = sig1 + sig2;
    else
      expectValue = sig1 - sig2;

    expectError = sqrt(err1*err1 + err2*err2);;

    double diff = err3 - expectError;
    if (diff < 0) diff = -diff;

    TSM_ASSERT_DELTA(message, sig3, expectValue, 0.0001);
    TSM_ASSERT_DELTA(message, err3, expectError, 0.0001);

    // Return false if the error is wrong
    return (diff < 0.0001);
  }






























  int numBins;
  int numPixels;
  std::string wsName_EW, wsName_2D, wsNameOut;
  EventWorkspace_sptr work_in1;
  Workspace2D_sptr work_in2;


  // Perform the test for given types
  void performTest_withClearRHS(MatrixWorkspace_sptr lhs, MatrixWorkspace_sptr rhs,
      bool clearRHS, bool expectEventOutput,
      size_t expectedOutputNumberEventsInOutput,
      bool rhsShouldBeCleared,
      int outputWorkspaceWillBe = 0
      )
  {
    lhs->setName("MinusTest_lhs");
    rhs->setName("MinusTest_rhs");
    switch (outputWorkspaceWillBe)
    {
    case 0:
      wsNameOut = "MinusTest_output";
      if (AnalysisDataService::Instance().doesExist(wsNameOut))
        AnalysisDataService::Instance().remove(wsNameOut);
      break;
    case 1: wsNameOut = "MinusTest_lhs"; break;
    case 2: wsNameOut = "MinusTest_rhs"; break;
    }

    TS_ASSERT_DELTA(  rhs->readY(0)[0], 2.00, 1e-5);
    TS_ASSERT_DELTA(  rhs->readE(0)[0], sqrt(2.00), 1e-5);

    //Do the minus
    IAlgorithm * alg;
    if (DO_PLUS)
      alg = new Plus();
    else
      alg = new Minus();

    alg->initialize();
    alg->setProperty("LHSWorkspace",lhs);
    alg->setProperty("RHSWorkspace",rhs);
    alg->setPropertyValue("OutputWorkspace",wsNameOut);
    alg->setProperty("ClearRHSWorkspace", clearRHS);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT( alg->isExecuted() );

    //The output!
    MatrixWorkspace_const_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<const MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsNameOut)));
    TS_ASSERT(work_out1);
    if (!work_out1)
      return;

    // The output is an EventWorkspace ?
    EventWorkspace_const_sptr eventOut = boost::dynamic_pointer_cast<const EventWorkspace>(work_out1);
    if (expectEventOutput)
    {
      TS_ASSERT(eventOut);
      if (!eventOut) return;
      TS_ASSERT_EQUALS( eventOut->getNumberEvents(), expectedOutputNumberEventsInOutput);
    }
    else
    {
      TS_ASSERT(!eventOut);
    }

    //Compare
    for (int pix=0; pix < numPixels; pix+=1)
      for (int i=0; i < numBins; i++)
      {
        if (DO_PLUS)
        {
          TS_ASSERT_DELTA(  work_out1->dataY(pix)[i], 4.00, 1e-5);
          TS_ASSERT_DELTA(  work_out1->dataE(pix)[i], sqrt(4.00), 1e-5);
        }
        else
        {
          TS_ASSERT_DELTA(  work_out1->dataY(pix)[i], 0.00, 1e-5);
          TS_ASSERT_DELTA(  work_out1->dataE(pix)[i], sqrt(4.00), 1e-5);
        }

        //Incoming event workspace should still have 2.0 for values
        TS_ASSERT_DELTA(  lhs->readY(pix)[i], 2.00, 1e-5);
        TS_ASSERT_DELTA(  lhs->readE(pix)[i], sqrt(2.0), 1e-5);

        if (!rhsShouldBeCleared)
        {
          //Incoming event workspace should still have 2.0 for values
          TS_ASSERT_DELTA(  rhs->readY(pix)[i], 2.00, 1e-5);
          TS_ASSERT_DELTA(  rhs->readE(pix)[i], sqrt(2.0), 1e-5);
        }
        else
        {
          // If you cleared it, should be 0
          TS_ASSERT_DELTA(  rhs->readY(pix)[i], 0.00, 1e-5);
          TS_ASSERT_DELTA(  rhs->readE(pix)[i], 0.00, 1e-5);
        }
      }

  }


  void test_EventWorkspace_minus_EventWorkspace()
  {
    EventWorkspace_sptr lhs = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    EventWorkspace_sptr rhs = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    performTest_withClearRHS(lhs,rhs, false, true, lhs->getNumberEvents() + rhs->getNumberEvents(), false);
  }

  void test_EventWorkspace_minus_EventWorkspace_clearRHS()
  {
    EventWorkspace_sptr lhs = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    EventWorkspace_sptr rhs = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    performTest_withClearRHS(lhs,rhs, true, true, lhs->getNumberEvents() + rhs->getNumberEvents(), true);
  }

  void test_Workspace2D_minus_EventWorkspace()
  {
    MatrixWorkspace_sptr lhs = WorkspaceCreationHelper::Create2DWorkspace(numPixels, numBins);
    EventWorkspace_sptr rhs = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    performTest_withClearRHS(lhs,rhs, false, false, 0, false);
  }

  void test_Workspace2D_minus_EventWorkspace_clearRHS()
  {
    MatrixWorkspace_sptr lhs = WorkspaceCreationHelper::Create2DWorkspace(numPixels, numBins);
    EventWorkspace_sptr rhs = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    performTest_withClearRHS(lhs,rhs, true, false, 0, true);
  }

  void test_EventWorkspace_minus_Workspace2D()
  {
    EventWorkspace_sptr lhs = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    MatrixWorkspace_sptr rhs = WorkspaceCreationHelper::Create2DWorkspace(numPixels, numBins);
    performTest_withClearRHS(lhs,rhs, false, false, 0, false);
  }

  void test_EventWorkspace_minus_Workspace2D_clearRHS()
  {
    EventWorkspace_sptr lhs = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    MatrixWorkspace_sptr rhs = WorkspaceCreationHelper::Create2DWorkspace(numPixels, numBins);
    performTest_withClearRHS(lhs,rhs, true, false, 0, false);
  }


  void test_EventWorkspace_minus_EventWorkspace_inPlace_of_lhs()
  {
    EventWorkspace_sptr lhs = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    EventWorkspace_sptr rhs = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    performTest_withClearRHS(lhs,rhs, false, true, lhs->getNumberEvents() + rhs->getNumberEvents(), false, 1);
  }

  void test_EventWorkspace_minus_EventWorkspace_inPlace_of_rhs()
  {
    EventWorkspace_sptr lhs = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    EventWorkspace_sptr rhs = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    performTest_withClearRHS(lhs,rhs, false, true, lhs->getNumberEvents() + rhs->getNumberEvents(), false, 2);
  }

  void test_EventWorkspace_minus_EventWorkspace_inPlace_AND_lhs_is_rhs()
  {
    EventWorkspace_sptr lhs = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    EventWorkspace_sptr rhs = lhs;
    performTest_withClearRHS(lhs,rhs, false, true, lhs->getNumberEvents() + rhs->getNumberEvents(), false, 1);
  }

  void test_EventWorkspace_minus_EventWorkspace_lhs_is_rhs()
  {
    EventWorkspace_sptr lhs = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    EventWorkspace_sptr rhs = lhs;
    performTest_withClearRHS(lhs,rhs, false, true, lhs->getNumberEvents() + rhs->getNumberEvents(), false);
  }

  void test_EventWorkspace_minus_EventWorkspace_lhs_is_rhs_with_clearRHS_set_doesnt_clearRHS()
  {
    EventWorkspace_sptr lhs = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    EventWorkspace_sptr rhs = lhs;
    performTest_withClearRHS(lhs,rhs, false, true, lhs->getNumberEvents() + rhs->getNumberEvents(), false);
  }

  void test_EventWorkspace_minus_EventWorkspace_inPlace_of_rhs_with_clearRHS_set_doesnt_clearRHS()
  {
    EventWorkspace_sptr lhs = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    EventWorkspace_sptr rhs = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    performTest_withClearRHS(lhs,rhs, false, true, lhs->getNumberEvents() + rhs->getNumberEvents(), false, 2);
  }





};


#endif

