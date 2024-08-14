// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
// clang-format off
#pragma once
#include <cxxtest/TestSuite.h>
#include <cmath>

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/Minus.h"
#include "MantidAlgorithms/Plus.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidHistogramData/HistogramBuilder.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceOpOverloads.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;



/********** PLEASE NOTE! THIS FILE WAS AUTO-GENERATED FROM CMAKE.  ***********************/
/********** Source = PlusMinusTest.in.h **************************************************/


class @PLUSMINUSTEST_CLASS@ : public CxxTest::TestSuite
{
public:

  bool DO_PLUS;
  std::string message;

  static @PLUSMINUSTEST_CLASS@ *createSuite() { return new @PLUSMINUSTEST_CLASS@(); }
  static void destroySuite( @PLUSMINUSTEST_CLASS@ *suite ) { delete suite; }

  @PLUSMINUSTEST_CLASS@()
  {
    numBins = 10;
    numPixels = 5;
    wsNameOut = "MinusTest_outputWorkspace";
    DO_PLUS = @PLUSMINUSTEST_DO_PLUS@;

    fibWS1d = WorkspaceCreationHelper::create1DWorkspaceFib(5, true);
    histWS_5x10_123 = WorkspaceCreationHelper::create2DWorkspace123(5,10, true);
    histWS_5x10_154 = WorkspaceCreationHelper::create2DWorkspace154(5,10, true);
    histWS_5x10_bin = WorkspaceCreationHelper::create2DWorkspace(5,10);
    eventWS_5x10_50 = WorkspaceCreationHelper::createEventWorkspace(5,10,50,0.0,1.0,2);
    eventWS_small = WorkspaceCreationHelper::createEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
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
    TS_ASSERT_THROWS( alg->setPropertyValue("LHSWorkspace","test_in21"), const std::invalid_argument &);
    TS_ASSERT_THROWS( alg->setPropertyValue("RHSWorkspace","test_in22"), const std::invalid_argument &);
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace","test_out2") );
    delete alg;
  }

  //====================================================================================
  //====================================================================================
  //====================================================================================

  void test_CompoundAssignment()
  {
    Workspace2D_sptr a = WorkspaceCreationHelper::create2DWorkspace(5,5);
    const Workspace_const_sptr b = a;
    Workspace2D_sptr c = WorkspaceCreationHelper::create2DWorkspace(5,5);
    if (DO_PLUS)
    {
      a += 5;
      TS_ASSERT_EQUALS(a->y(0)[0],7);
      TS_ASSERT_EQUALS(a,b);
      a += c;
      TS_ASSERT_EQUALS(a->y(0)[0],9);
      TS_ASSERT_EQUALS(a,b);
    }
    else
    {
      a -= 5;
      TS_ASSERT_EQUALS(a->y(0)[0],-3);
      TS_ASSERT_EQUALS(a,b);
      a -= c;
      TS_ASSERT_EQUALS(a->y(0)[0],-5);
      TS_ASSERT_EQUALS(a,b);
    }
  }

  void doRunTest(const std::string & lhs, const std::string & rhs,
                 const std::string & output, const double expectedCharge)
  {
    Plus alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(
    alg.setPropertyValue("LHSWorkspace",lhs);
    alg.setPropertyValue("RHSWorkspace",rhs);
    alg.setPropertyValue("OutputWorkspace",output);
    );
    alg.execute();

    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(output));

    TS_ASSERT_DELTA(work_out1->run().getProtonCharge(), expectedCharge, 1e-8);

  }

  /// The Plus algorithm sums values in the Run object. Minus does not.
  void test_RunAddition()
  {
    if (DO_PLUS)
    {
      MatrixWorkspace_sptr a = WorkspaceCreationHelper::createWorkspaceSingleValue(3);
      a->mutableRun().setProtonCharge(10.);
      MatrixWorkspace_sptr b = WorkspaceCreationHelper::createWorkspaceSingleValue(2);
      b->mutableRun().setProtonCharge(5.);
      AnalysisDataService::Instance().add("a", a);
      AnalysisDataService::Instance().add("b", b);

      doRunTest("a", "b", "c", 15.0);
      AnalysisDataService::Instance().remove("c");
      // In-place with LHS as output
      doRunTest("a", "b", "a", 15.0);
      // In-place with RHS as output
      doRunTest("a", "b", "b", 20.0);

      AnalysisDataService::Instance().remove("a");
      AnalysisDataService::Instance().remove("b");
    }
  }

  //====================================================================================
  //====================================================================================
  //====================================================================================

  void test_1D_1D()
  {
    MatrixWorkspace_sptr work_in1 = fibWS1d;
    MatrixWorkspace_sptr work_in2 = fibWS1d;
    performTest(work_in1,work_in2);
  }

  void test_1D_1DRand()
  {
    int nBins = 5;
    MatrixWorkspace_sptr work_in1 = fibWS1d;
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create1DWorkspaceRand(nBins, true);
    performTest(work_in1,work_in2);
  }

  void test_2D_2D()
  {
    MatrixWorkspace_sptr work_in1 = histWS_5x10_bin;
    MatrixWorkspace_sptr work_in2 = histWS_5x10_bin;
    performTest(work_in1,work_in2, false /*not inplace*/, false /*not event*/,
        DO_PLUS ? 4.0 : 0.0,   2.0);
  }

  void test_2D_2D_inplace()
  {
    int nHist = 5,nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspaceBinned(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = histWS_5x10_bin;
    performTest(work_in1,work_in2, true /*inplace*/, false /*not event*/,
        DO_PLUS ? 4.0 : 0.0,   2.0);
  }

  void test_2D_2D_NotHistograms()
  {
    MatrixWorkspace_sptr work_in1 = histWS_5x10_123;
    MatrixWorkspace_sptr work_in2 = histWS_5x10_154;
    performTest(work_in1,work_in2);
  }

  void test_2D_2D_Histograms()
  {
    int nHist = 5,nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace123(nHist,nBins, true);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create2DWorkspace154(nHist,nBins, true);
    performTest(work_in1,work_in2);
  }

  void test_1D_Rand2D()
  {
    int nHist = 5,nBins=5;
    const bool isHistogram(true);
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace154(nHist,nBins, isHistogram);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create1DWorkspaceRand(nBins, isHistogram);
    performTest(work_in1,work_in2);
  }

  void test_2D_1DVertical()
  {
    int nBins=10;
    MatrixWorkspace_sptr work_in1 = histWS_5x10_154;
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create2DWorkspace123(1,nBins, true);
    performTest(work_in1,work_in2);
  }

  void test_1DVertical_2D()
  {
    int nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace123(1,nBins, true);
    MatrixWorkspace_sptr work_in2 = histWS_5x10_154;
    if (DO_PLUS)
    {
      performTest(work_in1,work_in2,
          false /*in place*/, false /*not event*/,
          -1,-1, false, true /*algorithmWillCommute*/);
    }
    else
      performTest_fails(work_in1,work_in2);
  }

  void test_2D_2DSingleSpectrumBiggerSize_fails()
  {
    //In 2D workspaces, the X bins have to match
    int nHist = 10,nBins=5;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace123(nHist,nBins, true);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create2DWorkspace154(1,nBins*5);
    performTest_fails(work_in1, work_in2);
  }

  void test_2D_2DbyOperatorOverload()
  {
    MatrixWorkspace_sptr work_in1 = histWS_5x10_123;
    MatrixWorkspace_sptr work_in2 = histWS_5x10_154;
    const double value(8.0);
    MatrixWorkspace_sptr work_out1, work_out2;
    if (DO_PLUS)
    {
      work_out1 = work_in1+work_in2;
      work_out2 = work_in1+value;
    }
    else
    {
      work_out1 = work_in1-work_in2;
      work_out2 = work_in1-value;

      MatrixWorkspace_sptr work_out3 = value-work_in2;
      // checkData won't work on this one, do a few checks here
      TS_ASSERT_EQUALS( work_out3->size(), work_in2->size() );
      TS_ASSERT_EQUALS( work_out3->x(1), work_in2->x(1) );
      TS_ASSERT_EQUALS( work_out3->y(2)[6], 3.0 );
      TS_ASSERT_EQUALS( work_out3->e(3)[4], 4.0 );
    }

    checkData(work_in1, work_in2, work_out1);
    checkData(work_in1, std::make_shared<WorkspaceSingleValue>(value), work_out2);
  }

  void test_1D_SingleValue()
  {
    MatrixWorkspace_sptr work_in1 = fibWS1d;
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createWorkspaceSingleValue(2.2);
    performTest(work_in1,work_in2);
  }

  void test_SingleValue_1D()
  {
    int nBins = 5;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createWorkspaceSingleValue(2.2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create2DWorkspaceBinned(1,nBins);
    if (DO_PLUS)
      MatrixWorkspace_sptr out = performTest(work_in1,work_in2,
          false /*in place*/, false /*not event*/,
          -1, -1, false, true /*algorithmWillCommute*/);
    else
      performTest(work_in1,work_in2,
          false /*in place*/, false /*not event*/,
          0.2, 2.0493, false, true /*algorithmWillCommute*/);
  }

  void test_2D_SingleValue()
  {
    MatrixWorkspace_sptr work_in1 = histWS_5x10_bin;
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createWorkspaceSingleValue(4.455);
    performTest(work_in1,work_in2);
  }

  void test_2D_SingleValue_InPlace()
  {
    int nHist =5,nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspaceBinned(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createWorkspaceSingleValue(4.455);
    performTest(work_in1,work_in2, true /*in place*/, false /*not event*/,
        DO_PLUS ? 6.455 : -2.455,   2.5406);
  }

  void test_SingleValue_2D()
  {
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createWorkspaceSingleValue(4.455);
    MatrixWorkspace_sptr work_in2 = histWS_5x10_bin;
    if (DO_PLUS)
    {
      MatrixWorkspace_sptr out = performTest(work_in1,work_in2,
          false /*in place*/, false /*not event*/,
          -1,-1, false, true /*algorithmWillCommute*/);
      TS_ASSERT_EQUALS( out->getNumberHistograms(), 5);
      TS_ASSERT_EQUALS( out->blocksize(), 10);
    }
    else
      MatrixWorkspace_sptr out = performTest(work_in1,work_in2,
          false /*in place*/, false /*not event*/,
          2.455, 2.5406, false, true /*algorithmWillCommute*/);
  }

  void test_2D_SingleValueNoError()
  {
    MatrixWorkspace_sptr work_in1 = histWS_5x10_bin;
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createWorkspaceSingleValueWithError(5.0, 0.0);
    performTest(work_in1,work_in2);
  }

  //============================================================================================
  //========================================= EventWorkspaces ==================================
  //============================================================================================

  void test_Event_SingleValue()
  {
    MatrixWorkspace_sptr work_in1 = eventWS_5x10_50;
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createWorkspaceSingleValue(2.0);
    // Become a WS2D
    performTest(work_in1, work_in2, false, false /*output is NOT event*/ );
  }

  void test_Event_SingleValue_inPlace_fails()
  {
    MatrixWorkspace_sptr work_in1 = eventWS_5x10_50;
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createWorkspaceSingleValue(2.0);
    performTest_fails(work_in1, work_in2, true);
  }

  void test_SingleValue_Event()
  {
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createWorkspaceSingleValue(2.0);
    MatrixWorkspace_sptr work_in2 = eventWS_5x10_50;
    // Become a WS2D
    if (DO_PLUS)
    {
      MatrixWorkspace_sptr out = performTest(work_in1,work_in2,
          false /*in place*/, false /*not event*/,
          -1,-1, false, true /*algorithmWillCommute*/);
      TS_ASSERT_EQUALS( out->getNumberHistograms(), 5);
      TS_ASSERT_EQUALS( out->blocksize(), 10);
    }
    else
      performTest_fails(work_in1,work_in2);

  }

  void test_SingleValue_Event_inPlace_fails()
  {
    MatrixWorkspace_sptr work_in1 = eventWS_5x10_50;
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createWorkspaceSingleValue(2.0);
    // Become a WS2D
    performTest_fails(work_in1, work_in2, true);
  }



  void test_2D_Event()
  {
    MatrixWorkspace_sptr work_in1 = histWS_5x10_bin;
    MatrixWorkspace_sptr work_in2 = eventWS_5x10_50;
    performTest(work_in1,work_in2, false);
  }

  void test_2D_Event_inPlace()
  {
    int nHist = 5, nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = eventWS_5x10_50;
    // You have to specify the expected output value because in1 gets changed.
    performTest(work_in1,work_in2, true, false /*not event out*/,
        DO_PLUS ? 4.0 : 0.0, 2.0);
  }

  void test_Event_2D()
  {
    MatrixWorkspace_sptr work_in1 = eventWS_5x10_50;
    MatrixWorkspace_sptr work_in2 = histWS_5x10_bin;
    performTest(work_in1,work_in2, false);
  }

  void test_Event_2D_inPlace_fails()
  {
    MatrixWorkspace_sptr work_in1 = eventWS_5x10_50;
    MatrixWorkspace_sptr work_in2 = histWS_5x10_bin;
    performTest_fails(work_in1,work_in2, true);
  }



  void test_Event_2DSingleSpectrum()
  {
    MatrixWorkspace_sptr work_in1 = eventWS_5x10_50;
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create2DWorkspace(1, 10);
    performTest(work_in1,work_in2, false);
  }

  void test_Event_2DSingleSpectrum_inPlace_fails()
  {
    MatrixWorkspace_sptr work_in1 = eventWS_5x10_50;
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create2DWorkspace(1, 10);
    performTest_fails(work_in1,work_in2, true);
  }

  void test_2DSingleSpectrum_Event_fails()
  {
    for(int inplace=0; inplace<2;inplace++)
    {
      MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace(1, 10);
      MatrixWorkspace_sptr work_in2 = eventWS_5x10_50;
      if (DO_PLUS)
      {
        if (inplace!=0)
        {
          // This checks that 'in-place' succeeds, but the output workspace is a new one (with the same name)
          Plus plus;
          plus.initialize();
          plus.setProperty("LHSWorkspace",work_in1);
          plus.setProperty("RHSWorkspace",work_in2);
          AnalysisDataService::Instance().addOrReplace("outWS",work_in1);
          plus.setPropertyValue("OutputWorkspace","outWS");
          TS_ASSERT( plus.execute() );

          MatrixWorkspace_const_sptr bob = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
          // It's a different workspace to either of the inputs
          TS_ASSERT_DIFFERS( bob, work_in1 );
          TS_ASSERT_DIFFERS( bob, work_in2 );
          // Its dimensions match the RHS input
          TS_ASSERT_EQUALS( bob->size(), work_in2->size() );

          // Fails if the event workspace is on the left and you ask for it in place
          performTest_fails(work_in2,work_in1, inplace!=0);
        }
        else
          performTest(work_in1,work_in2, inplace!=0, false /*not event*/, 4.0, 2.0,
              false, true /*algorithmWillCommute*/);
      }
      else
        performTest_fails(work_in1,work_in2, inplace!=0);
    }
  }

  void test_Event_Event()
  {
    MatrixWorkspace_sptr work_in1 = eventWS_5x10_50;
    MatrixWorkspace_sptr work_in2 = eventWS_5x10_50;
    MatrixWorkspace_sptr work_out = performTest(work_in1,work_in2, false /*inPlace*/, true /*outputIsEvent*/,
        DO_PLUS ? 4.0 : 0.0, 2.0);
  }

  void test_Event_Event_inPlace()
  {
    int nHist = 5,nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createEventWorkspace(nHist,nBins,50,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = eventWS_5x10_50;
    MatrixWorkspace_sptr work_out = performTest(work_in1,work_in2, true, true /*outputIsEvent*/,
        DO_PLUS ? 4.0 : 0.0, 2.0);
  }

  void test_Event_EventSingleSpectrum_fails()
  {
    MatrixWorkspace_sptr work_in1 = eventWS_5x10_50;
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createEventWorkspace(1,10,50,0.0,1.0,2);
    performTest_fails(work_in1,work_in2, false);
  }

  void test_EventSingleSpectrum_Event_fails()
  {
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createEventWorkspace(1,10,50,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = eventWS_5x10_50;
    performTest_fails(work_in1,work_in2, false);
  }

  void test_EventWithASingleBin_EventWithASingleBin()
  {
    for(int inplace=0; inplace<2;inplace++)
    {
      int nHist = 5,nBins=1;
      MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createEventWorkspace(nHist,nBins,50,0.0,1.0,2);
      MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createEventWorkspace(nHist,nBins,50,0.0,1.0,2);
      MatrixWorkspace_sptr work_out = performTest(work_in1,work_in2, inplace!=0, true /*outputIsEvent*/,
          DO_PLUS ? 4.0 : 0.0, 2.0);
    }
  }

  void test_Event_EventWithASingleBin()
  {
    for(int inplace=0; inplace<2;inplace++)
    {
      int nHist = 5,nBins=10;
      MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createEventWorkspace(nHist,nBins,50,0.0,1.0,2);
      MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createEventWorkspace(nHist,1,50,0.0,1.0,2);
      MatrixWorkspace_sptr work_out = performTest(work_in1,work_in2, inplace!=0, true /*outputIsEvent*/,
          DO_PLUS ? 4.0 : 0.0, 2.0);
    }
  }

  void test_EventWithASingleBin_Event()
  {
    for(int inplace=0; inplace<2;inplace++)
    {
      MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createEventWorkspace(5,1,50,0.0,1.0,2);
      MatrixWorkspace_sptr work_in2 = eventWS_5x10_50;
      MatrixWorkspace_sptr work_out = performTest(work_in1,work_in2, inplace!=0, true /*outputIsEvent*/,
          DO_PLUS ? 4.0 : 0.0, 2.0);
    }
  }

  void test_EventWithASingleBinAndSingleSpectrum_EventWithASingleBinAndSingleSpectrum()
  {
    for(int inplace=0; inplace<2;inplace++)
    {
      int nHist=1,nBins=1;
      MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createEventWorkspace(nHist,nBins,50,0.0,1.0,2);
      MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createEventWorkspace(nHist,nBins,50,0.0,1.0,2);
      MatrixWorkspace_sptr work_out = performTest(work_in1,work_in2, inplace!=0, true /*outputIsEvent*/,
          DO_PLUS ? 4.0 : 0.0, 2.0);
    }
  }

  //====================================================================================
  //====================================================================================
  //====================================================================================

  //------------------------------------------------------------------------------------------------
  void test_Event_IncompatibleUnits_fails()
  {
    MatrixWorkspace_sptr work_in1 = eventWS_5x10_50;
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createEventWorkspace(5,10,50,0.0,1.0,2);
    work_in2->setYUnit("Microfurlongs per Megafortnights");
    performTest_fails(work_in1,work_in2, false /*not inplace*/);
  }


  //------------------------------------------------------------------------------------------------
  void test_Event_differentOutputAndDifferentPixelIDs()
  {
    for (int inplace =0; inplace < 2; inplace++)
    {
      MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createEventWorkspace(3,10,50, 0.0, 1.0, 3); // 5 ev
      MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createEventWorkspace(3,10,50, 0.0, 1.0, 2, 100); //100 events per spectrum, but the spectra are at different pixel ids

      //First pixel id of rhs is 100
      TS_ASSERT( work_in2->getSpectrum(0).hasDetectorID(100) );

      MatrixWorkspace_sptr work_out = performTest(work_in1,work_in2, inplace!=0, true /*outputIsEvent*/,
          DO_PLUS ? 3.0 : -1.0, 1.7320);

      //Ya, its an event workspace
      TS_ASSERT(work_out);
      if (!work_out) return;

      //But two detector IDs in each one
      for (int i=0; i<3; i++)
      {
        auto detIT = work_out->getSpectrum(i).getDetectorIDs().begin();
        TS_ASSERT_EQUALS( *detIT, 0+i );
        if (DO_PLUS)
        {
          detIT++;
          TS_ASSERT_EQUALS( *detIT, 100+i );
        }
      }
    }
  }

  //============================================================================

  std::string describe_workspace(const MatrixWorkspace_sptr ws)
  {
    std::ostringstream mess;
    EventWorkspace_const_sptr ews = std::dynamic_pointer_cast<const EventWorkspace>(ws);
    if (ews)
      mess << "Event";
    else
      mess << "2D";
    mess << "(" << ws->getNumberHistograms() << " spectra, ";
    if (ws->isRaggedWorkspace())
      mess << "Ragged";
    else
      mess << ws->blocksize();
    mess << " bins," << "Y[0][0] = " << ws->y(0)[0] << ")";
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
   * @return the created workspace
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
    EventWorkspace_const_sptr ews_1 = std::dynamic_pointer_cast<const EventWorkspace>(work_in1) ;
    if (ews_1) numEvents1 = ews_1->getNumberEvents();
    EventWorkspace_const_sptr ews_2 = std::dynamic_pointer_cast<const EventWorkspace>(work_in2) ;
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
    alg->setRethrows(true);
    TSM_ASSERT_THROWS_NOTHING(message, alg->execute());
    TSM_ASSERT( message, alg->isExecuted() );
    MatrixWorkspace_sptr work_out1;
    TSM_ASSERT_THROWS_NOTHING(message, work_out1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsNameOut));
    TSM_ASSERT( message, work_out1 );
    if (work_out1)
    {
      //Check that the output is an event workspace?
      if (outputIsEvent)
      {
        EventWorkspace_sptr ews_out = std::dynamic_pointer_cast<EventWorkspace>(work_out1) ;
        TSM_ASSERT( message, ews_out);
        // The # of events is equal to the sum of the original amount
        TSM_ASSERT_EQUALS( message, ews_out->getNumberEvents(), numEvents1 + numEvents2 );
      }
      else
      {
        // Check that it is NOT event
        TSM_ASSERT( message, !(std::dynamic_pointer_cast<EventWorkspace>(work_out1)) );
      }

      if (algorithmWillCommute)
      {
        checkData(work_in2, work_in1, work_out1, 0, expectedValue, expectedError);
      }
      else
      {
        checkData(work_in1, work_in2, work_out1, 0, expectedValue, expectedError);
      }

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
    if (work_out1->isRaggedWorkspace()) {
      TSM_ASSERT_LESS_THAN( message, 0, work_out1->y(0).size());
    } else {
      TSM_ASSERT_LESS_THAN( message, 0, work_out1->blocksize());
    }
    TSM_ASSERT_EQUALS( message, work_in1->getNumberHistograms(), work_out1->getNumberHistograms());
    // Number of histograms/bins is unchanged (relative to LHS argument)
    TSM_ASSERT_EQUALS( message, work_out1->getNumberHistograms(), work_in1->getNumberHistograms());
    if (work_out1->isRaggedWorkspace()) {
      TSM_ASSERT_EQUALS( message, work_out1->y(0).size(), work_in1->y(0).size());
    } else {
      TSM_ASSERT_EQUALS( message, work_out1->blocksize(), work_in1->blocksize());
    }

    if (expectedValue == -1.0 && expectedError == -1.0)
    {
      // --- Perform an automatic test ------------
      size_t ws2LoopCount = 0;
      if (work_in2->size() > 0)
      {
        ws2LoopCount = work_in1->size()/work_in2->size();
      }
      ws2LoopCount = (ws2LoopCount==0) ? 1 : ws2LoopCount;

      for (size_t i = 0; i < work_out1->size(); i++)
      {
        size_t ws2Index = i;

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
        {
          std::cout << "Failure at ws2Index " << ws2Index << std::endl;
          break;
        }
      }
    }
    else
    {
      // ------ Use expected answer --------------------
      bool breakOut=false;
      for (size_t wi=0; wi < work_out1->getNumberHistograms(); wi++)
      {
        const auto &xIn = work_in1->x(wi);
        const auto &xOut = work_out1->x(wi);
        const auto &yOut = work_out1->y(wi);
        const auto &eOut = work_out1->e(wi);
        const size_t numBins = yOut.size();
        for (size_t i=0; i<numBins; i++)
        {
          std::ostringstream mess;
          mess << message << ", evaluated at wi " << wi << ", i " << i;

          TS_ASSERT_DELTA(xIn[i], xOut[i], 0.0001);
          double sig3 = yOut[i];
          double err3 = eOut[i];
          TSM_ASSERT_DELTA(mess.str(), sig3, expectedValue, 0.0001);
          TSM_ASSERT_DELTA(mess.str(), err3, expectedError, 0.0001);
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
      size_t i, size_t ws2Index)
  {
    const size_t work_in1_blksize = work_in1->blocksize();
    const size_t work_in2_blksize = work_in2->blocksize();

    const double sig1 = work_in1->y(i/work_in1_blksize)[i%work_in1_blksize];
    const double sig2 = work_in2->y(ws2Index/work_in2_blksize)[ws2Index%work_in2_blksize];
    const double sig3 = work_out1->y(i/work_in1_blksize)[i%work_in1_blksize];

    TS_ASSERT_DELTA(work_in1->x(i/work_in1_blksize)[i%work_in1_blksize],
        work_out1->x(i/work_in1_blksize)[i%work_in1_blksize], 0.0001);

    const double err1 = work_in1->e(i/work_in1_blksize)[i%work_in1_blksize];
    const double err2 = work_in2->e(ws2Index/work_in2_blksize)[ws2Index%work_in2_blksize];
    const double err3 = work_out1->e(i/work_in1_blksize)[i%work_in1_blksize];

    double expectValue;
    //Compute the expectation
    if (DO_PLUS)
      expectValue = sig1 + sig2;
    else
      expectValue = sig1 - sig2;

    const double expectError = sqrt(err1*err1 + err2*err2);;

    TSM_ASSERT_DELTA(message, sig3, expectValue, 0.0001);
    TSM_ASSERT_DELTA(message, err3, expectError, 0.0001);

    // Return false if the error is wrong
    const double diff = std::abs(err3 - expectError);
    return (diff < 0.0001);
  }

  int numBins;
  int numPixels;
  std::string wsName_EW, wsName_2D, wsNameOut;
  MatrixWorkspace_sptr fibWS1d, histWS_5x10_123, histWS_5x10_154, histWS_5x10_bin, eventWS_5x10_50;
  EventWorkspace_sptr eventWS_small;


  // Perform the test for given types
  void performTest_withClearRHS(MatrixWorkspace_sptr lhs, MatrixWorkspace_sptr rhs,
      bool clearRHS, bool expectEventOutput,
      size_t expectedOutputNumberEventsInOutput,
      bool rhsShouldBeCleared,
      int outputWorkspaceWillBe = 0
      )
  {
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

	// Sanity check, the inputs were set to have 2.0 in each bin (2 events)
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
    TS_ASSERT_THROWS_NOTHING(work_out1 = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(wsNameOut));
    TS_ASSERT(work_out1);
    if (!work_out1)
      return;

    // The output is an EventWorkspace ?
    EventWorkspace_const_sptr eventOut = std::dynamic_pointer_cast<const EventWorkspace>(work_out1);
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

    delete alg;
  }


  void test_EventWorkspace_EventWorkspace()
  {
    EventWorkspace_sptr lhs = eventWS_small;
    EventWorkspace_sptr rhs = eventWS_small;
    performTest_withClearRHS(lhs,rhs, false /*clearRHS*/,
        true /*expectEventOutput*/,
        lhs->getNumberEvents() + rhs->getNumberEvents(),
        false /*rhsShouldBeCleared*/);
  }

  void test_EventWorkspace_EventWorkspace_clearRHS()
  {
    EventWorkspace_sptr lhs = eventWS_small;
    EventWorkspace_sptr rhs = WorkspaceCreationHelper::createEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    performTest_withClearRHS(lhs,rhs, true, true, lhs->getNumberEvents() + rhs->getNumberEvents(), true);
  }

  void test_Workspace2D_EventWorkspace()
  {
    MatrixWorkspace_sptr lhs = histWS_5x10_bin;
    EventWorkspace_sptr rhs = eventWS_small;
    performTest_withClearRHS(lhs,rhs, false, false, 0, false);
  }

  void test_Workspace2D_EventWorkspace_clearRHS()
  {
    MatrixWorkspace_sptr lhs = histWS_5x10_bin;
    EventWorkspace_sptr rhs = WorkspaceCreationHelper::createEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    performTest_withClearRHS(lhs,rhs, true, false, 0, true);
  }

  void test_EventWorkspace_Workspace2D()
  {
    EventWorkspace_sptr lhs = eventWS_small;
    MatrixWorkspace_sptr rhs = histWS_5x10_bin;
    performTest_withClearRHS(lhs,rhs, false, false, 0, false);
  }

  void test_EventWorkspace_Workspace2D_clearRHS()
  {
    EventWorkspace_sptr lhs = eventWS_small;
    MatrixWorkspace_sptr rhs = WorkspaceCreationHelper::create2DWorkspace(numPixels, numBins);
    performTest_withClearRHS(lhs,rhs, true, false, 0, false);
  }


  void test_EventWorkspace_EventWorkspace_inPlace_of_lhs()
  {
    EventWorkspace_sptr lhs = WorkspaceCreationHelper::createEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    EventWorkspace_sptr rhs = WorkspaceCreationHelper::createEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    performTest_withClearRHS(lhs,rhs, false, true, lhs->getNumberEvents() + rhs->getNumberEvents(), false, 1);
  }

  void test_EventWorkspace_EventWorkspace_inPlace_of_rhs()
  {
    EventWorkspace_sptr lhs = WorkspaceCreationHelper::createEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    EventWorkspace_sptr rhs = WorkspaceCreationHelper::createEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    performTest_withClearRHS(lhs,rhs, false, true, lhs->getNumberEvents() + rhs->getNumberEvents(), false, 2);
  }

  void test_EventWorkspace_EventWorkspace_inPlace_AND_lhs_is_rhs()
  {
    EventWorkspace_sptr lhs = WorkspaceCreationHelper::createEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    EventWorkspace_sptr rhs = lhs;
    performTest_withClearRHS(lhs,rhs, false, true, lhs->getNumberEvents() + rhs->getNumberEvents(), false, 1);
  }

  void test_EventWorkspace_EventWorkspace_lhs_is_rhs()
  {
    EventWorkspace_sptr lhs = eventWS_small;
    EventWorkspace_sptr rhs = lhs;
    performTest_withClearRHS(lhs,rhs, false, true, lhs->getNumberEvents() + rhs->getNumberEvents(), false);
  }

  void test_EventWorkspace_EventWorkspace_lhs_is_rhs_with_clearRHS_set_doesnt_clearRHS()
  {
    EventWorkspace_sptr lhs = WorkspaceCreationHelper::createEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    EventWorkspace_sptr rhs = lhs;
    performTest_withClearRHS(lhs,rhs, false, true, lhs->getNumberEvents() + rhs->getNumberEvents(), false);
  }

  void test_EventWorkspace_EventWorkspace_inPlace_of_rhs_with_clearRHS_set_doesnt_clearRHS()
  {
    EventWorkspace_sptr lhs = WorkspaceCreationHelper::createEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    EventWorkspace_sptr rhs = WorkspaceCreationHelper::createEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
    performTest_withClearRHS(lhs,rhs, false, true, lhs->getNumberEvents() + rhs->getNumberEvents(), false, 2);
  }


  MatrixWorkspace_sptr create_RaggedWorkspace()
  {
    // create workspace with 2 histograms
    MatrixWorkspace_sptr raggedWS = WorkspaceCreationHelper::create2DWorkspace(2, 1);

    // create and replace histograms with ragged ones
    MantidVec x_data{100., 200., 300., 400.};
    MantidVec y_data{1., 1., 1.};
    MantidVec e_data{1., 1., 1.};
    Mantid::HistogramData::HistogramBuilder builder;
    builder.setX(x_data);
    builder.setY(y_data);
    builder.setE(e_data);
    raggedWS->setHistogram(0, builder.build());

    MantidVec x_data2{200., 400., 600.};
    MantidVec y_data2{1., 1.};
    MantidVec e_data2{1., 1.};
    Mantid::HistogramData::HistogramBuilder builder2;
    builder2.setX(x_data2);
    builder2.setY(y_data2);
    builder2.setE(e_data2);
    raggedWS->setHistogram(1, builder2.build());

    // quick check of the workspace
    TS_ASSERT(raggedWS->isRaggedWorkspace());
    TS_ASSERT_EQUALS(raggedWS->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(raggedWS->x(0).size(), 4);
    TS_ASSERT_EQUALS(raggedWS->x(1).size(), 3);
    TS_ASSERT_EQUALS(raggedWS->y(0).size(), 3);
    TS_ASSERT_EQUALS(raggedWS->y(1).size(), 2);
    return raggedWS;
  }

  void test_RaggedWorkspace()
  {
    auto lhs = create_RaggedWorkspace();
    auto rhs = create_RaggedWorkspace();
    auto result = performTest(lhs, rhs, false, false, DO_PLUS ? 2.0 : 0.0, 1.4142135625);
    TS_ASSERT(result->isRaggedWorkspace());
  }

  void test_RaggedWorkspace_and_single_value()
  {
    auto lhs = create_RaggedWorkspace();
    auto rhs = WorkspaceCreationHelper::createWorkspaceSingleValue(2);
    auto result = performTest(lhs, rhs, false, false, DO_PLUS ? 3.0 : -1.0, 1.7320508071);
    TS_ASSERT(result->isRaggedWorkspace());
  }

  void test_RaggedWorkspace_not_compatible_x()
  {
    auto lhs = create_RaggedWorkspace();
    auto rhs = WorkspaceCreationHelper::create2DWorkspace(2, 4);
    performTest_fails(lhs, rhs);
  }

}; // end of class @PLUSMINUSTEST_CLASS@


//============================================================================
/** Performance test with large workspaces. */

class @PLUSMINUSTEST_CLASS@Performance : public CxxTest::TestSuite
{
  Workspace2D_sptr m_ws2D_1, m_ws2D_2;

public:
  static @PLUSMINUSTEST_CLASS@Performance *createSuite() { return new @PLUSMINUSTEST_CLASS@Performance(); }
  static void destroySuite( @PLUSMINUSTEST_CLASS@Performance *suite ) { delete suite; }

  void setUp() override
  {
    constexpr int histograms{100000};
    constexpr int bins{1000};
    m_ws2D_1 = WorkspaceCreationHelper::create2DWorkspace(histograms, bins);
    m_ws2D_2 = WorkspaceCreationHelper::create2DWorkspace(histograms, bins);
  }

  void test_large_2D()
  {
    constexpr bool doPlus{@PLUSMINUSTEST_DO_PLUS@};
    if (doPlus) {
      MatrixWorkspace_sptr out = m_ws2D_1 + m_ws2D_2;
    } else {
      MatrixWorkspace_sptr out = m_ws2D_1 - m_ws2D_2;
    }
  }

}; // end of class @PLUSMINUSTEST_CLASS@Performance


