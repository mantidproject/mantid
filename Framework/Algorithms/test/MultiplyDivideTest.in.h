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
#include <stdexcept>

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/Divide.h"
#include "MantidAlgorithms/Multiply.h"
#include "MantidHistogramData/HistogramBuilder.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidDataObjects/EventWorkspaceHelpers.h"
#include "MantidKernel/OptionalBool.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using Mantid::Geometry::IDetector_const_sptr;


/********** PLEASE NOTE! THIS FILE WAS AUTO-GENERATED FROM CMAKE.  ***********************/
/********** Source = MultiplyDivideTest.in.h *********************************************/


class @MULTIPLYDIVIDETEST_CLASS@ : public CxxTest::TestSuite
{
private:
  MatrixWorkspace_sptr fibWS1d, histWS_5x10_123, histWS_5x10_154, histWS_5x10_bin, eventWS_5x10_50;

public:
  bool DO_DIVIDE;
  std::string message;

  static @MULTIPLYDIVIDETEST_CLASS@ *createSuite() { return new @MULTIPLYDIVIDETEST_CLASS@(); }
  static void destroySuite( @MULTIPLYDIVIDETEST_CLASS@ *suite ) { delete suite; }

  @MULTIPLYDIVIDETEST_CLASS@()
  {
    DO_DIVIDE = @MULTIPLYDIVIDETEST_DO_DIVIDE@;

    fibWS1d = WorkspaceCreationHelper::create1DWorkspaceFib(5, true);
    histWS_5x10_123 = WorkspaceCreationHelper::create2DWorkspace123(5,10, true);
    histWS_5x10_154 = WorkspaceCreationHelper::create2DWorkspace154(5,10, true);
    histWS_5x10_bin = WorkspaceCreationHelper::create2DWorkspace(5,10);
    eventWS_5x10_50 = WorkspaceCreationHelper::createEventWorkspace(5,10,50,0.0,1.0,2);
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
    TS_ASSERT_THROWS( alg->setPropertyValue("LHSWorkspace","test_in21"), const std::invalid_argument &);
    TS_ASSERT_THROWS( alg->setPropertyValue("RHSWorkspace","test_in22"), const std::invalid_argument &);
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace","test_out2") );
    delete alg;
  }

  void testDivideWithMaskedSpectraProducesZeroes()
  {
    doDivideWithMaskedTest(false);
  }

  void testDivideWithMaskedSpectraProducesZeroesWhenReplacingInputWorkspace()
  {
    doDivideWithMaskedTest(true);
  }

  void testDivideForceIsDistributionTrue()
  {
    if (DO_DIVIDE) {
      MatrixWorkspace_sptr numerator = WorkspaceCreationHelper::create2DWorkspace(10, 2);
      MatrixWorkspace_sptr denominator = WorkspaceCreationHelper::createWorkspaceSingleValue(1.0);

      Divide alg;
      alg.initialize();
      alg.setChild(true);
      alg.setProperty("LHSWorkspace", numerator);
      alg.setProperty("RHSWorkspace", denominator);
      alg.setPropertyValue("OutputWorkspace", "dummy");
      alg.setProperty("IsDistribution", OptionalBool(true));
      alg.execute();

      MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
      TS_ASSERT_EQUALS(outWS->isDistribution(), true);
    }
  }
  void testDivideForceIsDistributionFalse()
  {
    if (DO_DIVIDE) {

      MatrixWorkspace_sptr numerator = WorkspaceCreationHelper::createWorkspaceSingleValue(10.0);
      MatrixWorkspace_sptr denominator = WorkspaceCreationHelper::createWorkspaceSingleValue(1.0);

      Divide alg;
      alg.initialize();
      alg.setChild(true);
      alg.setProperty("LHSWorkspace", numerator);
      alg.setProperty("RHSWorkspace", denominator);
      alg.setPropertyValue("OutputWorkspace", "dummy");
      alg.setProperty("IsDistribution", OptionalBool(false));
      alg.execute();

      MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
      TS_ASSERT_EQUALS(outWS->isDistribution(), false);
    }
  }

  void testCompoundAssignment()
  {

    MatrixWorkspace_sptr a = WorkspaceCreationHelper::createWorkspaceSingleValue(3);
    const Workspace_const_sptr b = a;
    MatrixWorkspace_sptr c = WorkspaceCreationHelper::createWorkspaceSingleValue(2);
    if (DO_DIVIDE)
    {
      a /= 5;
      TS_ASSERT_EQUALS(a->y(0)[0], 0.6)
      TS_ASSERT_EQUALS(a,b);
      a /= c;
      TS_ASSERT_EQUALS(a->y(0)[0], 0.3);
      TS_ASSERT_EQUALS(a,b);
    }
    else
    {
      a *= 5;
      TS_ASSERT_EQUALS(a->y(0)[0],15.0)
      TS_ASSERT_EQUALS(a,b);
      a *= c;
      TS_ASSERT_EQUALS(a->y(0)[0],30.0);
      TS_ASSERT_EQUALS(a,b);
    }
  }

  //========================================= 2D and 1D Workspaces ==================================

  void test_1D_1D()
  {
    MatrixWorkspace_sptr work_in1 = fibWS1d;
    MatrixWorkspace_sptr work_in2 = fibWS1d;
    performTest(work_in1,work_in2);
  }

  void test_2D_2D()
  {
    MatrixWorkspace_sptr work_in1 = histWS_5x10_123;
    MatrixWorkspace_sptr work_in2 = histWS_5x10_154;
    performTest(work_in1,work_in2);
  }

  void test_2D_2D_inPlace()
  {
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace(5,10);
    MatrixWorkspace_sptr work_in2 = histWS_5x10_bin;
    performTest(work_in1,work_in2, false /*not event*/,
        DO_DIVIDE ? 1.0 : 4.0, DO_DIVIDE ? 1.0 : 4.0, false, false, true /*in place*/);
  }

  void test_2D_1D_different_spectrum_number()
  {
    if(DO_DIVIDE)
    {
    int nHist = 5,nBins=5;
    MatrixWorkspace_sptr numerator  = WorkspaceCreationHelper::create2DWorkspace123(nHist-1,nBins); // Cropped
    MatrixWorkspace_sptr denominator = WorkspaceCreationHelper::create2DWorkspace123(nHist, 1); // Integrated
    Divide alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("LHSWorkspace", numerator);
    alg.setProperty("RHSWorkspace", denominator);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.setProperty("AllowDifferentNumberSpectra", true);
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), nHist-1);
    }
  }

  void test_2D_denominator_with_fewer_spectra()
  {
    if(DO_DIVIDE)
    {
    int nHist = 5,nBins=5;
    MatrixWorkspace_sptr numerator  = WorkspaceCreationHelper::create2DWorkspace123(nHist,nBins);
    MatrixWorkspace_sptr denominator = WorkspaceCreationHelper::create2DWorkspace123(nHist-1, nBins); // Cropped
    Divide alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("LHSWorkspace", numerator);
    alg.setProperty("RHSWorkspace", denominator);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.setProperty("AllowDifferentNumberSpectra", true);
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), nHist);
    TS_ASSERT_EQUALS(outWS->y(0)[0], 1.0);
    TS_ASSERT_EQUALS(outWS->y(1)[0], 1.0);
    TS_ASSERT_EQUALS(outWS->y(2)[0], 1.0);
    TS_ASSERT_EQUALS(outWS->y(3)[0], 1.0);
    TS_ASSERT_EQUALS(outWS->y(4)[0], 0.0);
    }
  }

  void test_2D_1DColumn()
  {
    for (int inplace=0; inplace<2; inplace++)
    {
      int nHist = 5,nBins=10;
      MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace(nHist,nBins);
      MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create2DWorkspace(nHist,1);
      performTest(work_in1,work_in2, false /*not event*/,
          DO_DIVIDE ? 1.0 : 4.0, DO_DIVIDE ? 1.0 : 4.0, false, false, inplace!=0 /*in place*/);
    }
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
    MatrixWorkspace_sptr work_in1 = histWS_5x10_154;
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create2DWorkspace123(1,10, true);
    performTest(work_in1,work_in2);
  }

  void test_2D_2DSingleSpectrumBiggerSize_fails()
  {
    //In 2D workspaces, the X bins have to match
    int nHist = 20,nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace123(nHist,nBins, true);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create2DWorkspace154(1,nBins*5, true);
    performTest_fails(work_in1, work_in2);
  }

  void test_2D_2DbyOperatorOverload()
  {
    MatrixWorkspace_sptr work_in1 = histWS_5x10_123;
    MatrixWorkspace_sptr work_in2 = histWS_5x10_154;
    MatrixWorkspace_sptr work_out1, work_out2, work_out3;
    const double value(3.0);
    if (DO_DIVIDE)
    {
      work_out1 = work_in1/work_in2;
      work_out2 = work_in1/value;
      work_out3 = value/work_in2;
      // checkData won't work on this one, do a few checks here
      TS_ASSERT_EQUALS( work_out3->size(), work_in2->size() );
      TS_ASSERT_EQUALS( work_out3->x(1), work_in2->x(1) );
      TS_ASSERT_DELTA( work_out3->y(2)[6], 0.6,  0.0001 );
      TS_ASSERT_DELTA( work_out3->e(3)[4], 0.48, 0.0001 );
    }
    else
    {
      work_out1 = work_in1*work_in2;
      work_out2 = work_in1*value;
      work_out3 = value*work_in2;
      checkData(work_in2, std::make_shared<WorkspaceSingleValue>(value), work_out3);
    }

    checkData(work_in1, work_in2, work_out1);
    checkData(work_in1, std::make_shared<WorkspaceSingleValue>(value), work_out2);
  }

  void test_2D_2DbyOperatorOverload_inPlace()
  {
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace(5,10);
    MatrixWorkspace_sptr work_in2 = histWS_5x10_bin;
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
    MatrixWorkspace_sptr work_in1 = fibWS1d;
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createWorkspaceSingleValue(2.2);
    performTest(work_in1,work_in2);
  }

  void test_SingleValue_1D()
  {
    int nBins = 5;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createWorkspaceSingleValue(10.0);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create2DWorkspace(1,nBins);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2,false,
          5.0, 3.8729, false, true /*commutes*/);
    else
      performTest(work_in1,work_in2,false,-1,-1,false,true); // will commute L and R
  }

  void test_2D_SingleValue()
  {
    for (int inplace=0; inplace<2; inplace++)
    {
      int nHist = 5,nBins=10;
      MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace(nHist,nBins);
      MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createWorkspaceSingleValue(2.0);
      performTest(work_in1,work_in2, false /*not event*/,
          DO_DIVIDE ? 1.0 : 4.0, DO_DIVIDE ? 1.0 : 4.0, false, false, inplace!=0 /*in place*/);
    }
  }

  void test_SingleValue_2D()
  {
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createWorkspaceSingleValue(10.0);
    MatrixWorkspace_sptr work_in2 = histWS_5x10_bin;
    if (DO_DIVIDE)
      performTest(work_in1,work_in2,false,
          5.0, 3.8729, false, true /*commutes*/);
    else
      performTest(work_in1,work_in2,false,-1,-1,false,true); // will commute L and R
  }

  void test_2D_SingleValueNoError()
  {
    MatrixWorkspace_sptr work_in1 = histWS_5x10_bin;
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createWorkspaceSingleValueWithError(5.0, 0.0);
    performTest(work_in1,work_in2);
  }




  //========================================= EventWorkspaces ==================================

  // Equivalent of 2D * or / 2D, really
  void test_2D_Event()
  {
    MatrixWorkspace_sptr work_in1 = histWS_5x10_bin;
    MatrixWorkspace_sptr work_in2 = eventWS_5x10_50;
    performTest(work_in1,work_in2, false);
  }

  void test_1DVertical_EventWithOneBin_willCommute()
  {
    int nBins=1,nHist=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createEventWorkspace(nHist, nBins,50,0.0,1.0,2);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, false /*output is not Event */, 1.0, 1.0, false, false, false /* not in place */);
    else
      performTest(work_in1,work_in2, true /*output is Event */, 4.0, 3.4641, false, true /*will commute*/, false /* not in place */);
  }

  void test_1DVertical_EventWithOneBin_willCommute_inplace()
  {
    int nBins=1,nHist=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createEventWorkspace(nHist, nBins,50,0.0,1.0,2);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, false /*output is not Event */, 1.0, 1.0, false, false, true /*in place*/);
    else
      performTest(work_in1,work_in2, true /*output is Event */, 4.0, 3.4641, false, false /*will commute*/, true /*in place*/);
  }

  void test_2D_Event_inPlace()
  {
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace(5,10);
    MatrixWorkspace_sptr work_in2 = eventWS_5x10_50;
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, false /*output is not Event */, 1.0, sqrt(1.0), false, false, true);
    else
      performTest(work_in1,work_in2, false /*output is not Event */, 4.0, 4.0, false, false, true);
  }

  void test_2D_Event_RHSEventWorkspaceHasOnebin()
  {
    MatrixWorkspace_sptr work_in1 = histWS_5x10_bin;
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createEventWorkspace(5, 1,50,0.0,100.0,2);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, false /*output is not Event */, 1.0, sqrt(1.0), false, false, false);
    else
      performTest(work_in1,work_in2, false /*output is not Event */, 4.0, 4.0, false, false /*no commute*/, false);
  }

  void test_2D_Event_inPlace_RHSEventWorkspaceHasOnebin()
  {
    int nHist = 5,nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createEventWorkspace(nHist, 1,50,0.0,100.0,2);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, false /*output is not Event */, 1.0, sqrt(1.0), false, false, true);
    else
      performTest(work_in1,work_in2, false /*output is not Event */, 4.0, 4.0, false, false /*no commute*/, true);
  }

  void test_2D_Event_inPlace_RHSEventWorkspaceHasOnebinAndOneSpectrum()
  {
    int nHist = 5,nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createEventWorkspace(1, 1,50,0.0,100.0,2);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, false /*output is not Event*/, 1.0, sqrt(1.0), false, false, true);
    else
      performTest(work_in1,work_in2, false /*output is not Event*/, 4.0, 4.0, false, false /*no commute*/, true);
  }

  void test_Event_2D_inplace_LHSEventWorkspaceHasOnebin()
  {
    int nHist = 5,nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createEventWorkspace(nHist, 1, 2, 0.0, 1.0, 2); // Events are at 0.5
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create2DWorkspace(nHist,nBins);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true /*output is Event */, 1.0, 0.8660, false, false, true);
    else
      // MULTIPLY: This commutes because the RHS workspace is bigger; the LHS workspace is treated as single number
      performTest(work_in1,work_in2, false /*output is not Event */, 4.0, 4.0, false, true /* commute */, true);
  }

  void test_Event_2D_inplace_LHSEventWorkspaceHasOnebinAndOneSpectrum()
  {
    int nHist = 5,nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createEventWorkspace(1, 1, 2, 0.0, 1.0, 2); // Events are at 0.5
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create2DWorkspace(nHist,nBins);
    if (DO_DIVIDE)
      performTest_fails(work_in1,work_in2); // Incompatible sizes
    else
      // MULTIPLY: This commutes because the RHS workspace is bigger; the LHS workspace is treated as single number
      performTest(work_in1,work_in2, false /*output is not Event */, 4.0, 4.0, false, true /* will commute */, true);
  }

  void test_Event_2D()
  {
    MatrixWorkspace_sptr work_in1 = eventWS_5x10_50;
    MatrixWorkspace_sptr work_in2 = histWS_5x10_bin;
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75));
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0));
  }

  void test_Event_2D_inPlace()
  {
    int nHist = 5,nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createEventWorkspace(nHist,nBins,50,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create2DWorkspace(nHist,nBins);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75), false, false, true);
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0), false, false, true);
  }

  void test_Event_2DSingleSpectrum()
  {
    MatrixWorkspace_sptr work_in1 = eventWS_5x10_50;
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create2DWorkspace(1, 10);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75));
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0));
  }

  void test_Event_2DSingleSpectrum_inPlace()
  {
    int nHist = 10,nBins=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createEventWorkspace(nHist,nBins,100,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create2DWorkspace(1, nBins);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75), false, false, true /* in-place */);
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0), false, false, true /* in-place */);
  }

  void test_Event_2DSingleSpectrumBiggerSize()
  {
    //Unlike 2D workspaces, you can divide by a single spectrum with different X bins!
    MatrixWorkspace_sptr work_in1 = eventWS_5x10_50;
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create2DWorkspace(1, 5*2);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75));
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0));
  }

  void test_2DSingleSpectrum_Event()
  {
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace(1, 10);
    MatrixWorkspace_sptr work_in2 = eventWS_5x10_50;
    if (DO_DIVIDE)
      performTest_fails(work_in1,work_in2); /* Fails for dividing, since you can't commute */
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0), false, true /*will commute */, false);
  }

  void test_2DSingleSpectrum_Event_inPlace()
  {
    int nHist = 5,nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace(1, nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createEventWorkspace(nHist,nBins,50,0.0,1.0,2);
    if (DO_DIVIDE)
      performTest_fails(work_in1,work_in2); /* Fails for dividing, since you can't commute */
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0), false, true /*will commute */, true /* in-place */);
  }

  void test_2DSingleSpectrumBiggerSize_Event()
  {
    //Unlike 2D workspaces, you can divide by a single spectrum with different X bins!
    int nBins = 5,nHist=5;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace(1, nHist*2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createEventWorkspace(nBins,nHist,50,0.0,1.0,2);
    if (DO_DIVIDE)
      performTest_fails(work_in1,work_in2); /* Fails for dividing, since you can't commute */
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0), false, true /*will commute */, false);
  }

  void test_2DSingleSpectrumBiggerSize_Event_inPlace()
  {
    //Unlike 2D workspaces, you can divide by a single spectrum with different X bins!
    int nBins = 5,nHist=5;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace(1, nBins*2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createEventWorkspace(nBins,nHist,50,0.0,1.0,2);
    if (DO_DIVIDE)
      performTest_fails(work_in1,work_in2); /* Fails for dividing, since you can't commute */
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0), false, true /*will commute */, false);
  }

  void test_Event_SingleValue()
  {
    MatrixWorkspace_sptr work_in1 = eventWS_5x10_50;
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createWorkspaceSingleValue(2.0);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75));
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0));
  }

  void test_Event_SingleValue_inPlace()
  {
    int nHist = 5,nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createEventWorkspace(nHist,nBins,50,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createWorkspaceSingleValue(2.0);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75), false, false, true /* in-place */);
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0), false, false, true /* in-place */);
  }

  void test_SingleValue_Event()
  {
    int nHist = 5,nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createWorkspaceSingleValue(10.0);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createEventWorkspace(nHist,nBins,50,0.0,1.0,2);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, false /*NOT events*/,
          5.0, 3.8729, false, true /*commutes*/);
    else
      performTest(work_in1,work_in2, true, 20.0, 14.8323, false, true /* will commute */);
  }

  void test_SingleValue_Event_inPlace()
  {
    // Doing in-place on a single value is silly since it just gets overwritten, but it works!
    int nHist = 5,nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createWorkspaceSingleValue(2.0);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createEventWorkspace(nHist,nBins,50,0.0,1.0,2);
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, false /*NOT events*/,
          1.0, 1.0, false, true /*commutes*/);
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0), false, true /* will commute */, true);
  }

  void test_Event_SingleValueNoError()
  {
    MatrixWorkspace_sptr work_in1 = eventWS_5x10_50;
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createWorkspaceSingleValueWithError(2.0, 0.0);
    performTest(work_in1,work_in2, true);
  }

  void test_Event_Event()
  {
    MatrixWorkspace_sptr work_in1 = eventWS_5x10_50;
    MatrixWorkspace_sptr work_in2 = eventWS_5x10_50;
    if (DO_DIVIDE)
      performTest(work_in1,work_in2, true, 1.0, sqrt(0.75));
    else
      performTest(work_in1,work_in2, true, 4.0, sqrt(12.0));
  }

  void test_Event_Event_inPlace()
  {
    int nHist = 5,nBins=10;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createEventWorkspace(nHist,nBins,50,0.0,1.0,2);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createEventWorkspace(nHist,nBins,50,0.0,1.0,2);
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

    if (lhs_grouping == 0 || rhs_grouping == 0){
      throw std::runtime_error("Attempted div by zero in test");
    }

    int numpix = 12;
    std::vector< std::vector<int> > lhs(numpix/lhs_grouping), rhs(numpix/rhs_grouping);
    for (int i=0; i<numpix; i++)
    {
      // lhs_grouping detectors in each on the lhs
      lhs[i/lhs_grouping].push_back(i);
      // rhs_grouping detectors in each on the lhs
      rhs[i/rhs_grouping].push_back(i);
    }
    // Grouped workspace will have lhs_grouping events in each bin (also).
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::createGroupedEventWorkspace(lhs, 10, 1.0);
    if (lhs2D)
      work_in1 = EventWorkspaceHelpers::convertEventTo2D(work_in1);
    TS_ASSERT_DELTA( work_in1->e(0)[0], sqrt( double(lhs_grouping*1.0) ), 1e-5);

    // Grouped workspace will have rhs_grouping events in each bin (also).
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::createGroupedEventWorkspace(rhs, 10, 1.0);
    if (rhs2D)
      work_in2 = EventWorkspaceHelpers::convertEventTo2D(work_in2);
    TS_ASSERT_DELTA( work_in2->e(0)[0], sqrt( double(rhs_grouping*1.0) ), 1e-5);

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
        doGroupedTest(1, lhs_2D!=0, 2, rhs_2D!=0, 0.5, sqrt(0.375),   2.0, sqrt(6.0));
  }


  void test_Grouped_Grouped()
  {
    for (int lhs_2D=0; lhs_2D<2; lhs_2D++)
      for (int rhs_2D=0; rhs_2D<2; rhs_2D++)
        if (lhs_2D)
          doGroupedTest(2, lhs_2D!=0, 4, rhs_2D!=0, 0.5, 0.4330,    8.0, sqrt(48.0));
        else
          // Errors are different when LHS is events!
          doGroupedTest(2, lhs_2D!=0, 4, rhs_2D!=0, 0.5, 0.3952,    8.0, sqrt(40.0));
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
            doGroupedTest(2, lhs_2D!=0, 1, rhs_2D!=0, 2.0, sqrt(2.0),    2.0, sqrt(6.0));
          else
            doGroupedTest(2, lhs_2D!=0, 1, rhs_2D!=0, 2.0, sqrt(2.0),    2.0, sqrt(6.0));
        }
  }

  void test_GroupedEvent_NotGrouped2D()
  {
    doGroupedTest(2, false, 1, true, 2.0, sqrt(2.0),    2.0, sqrt(6.0));
  }


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
    mess << " bins,";
    mess << "Y[0][0] = " << ws->y(0)[0] << ")";
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
        TSM_ASSERT( message, std::dynamic_pointer_cast<EventWorkspace>(work_out1) );
      }
      else
      {
        // Check that it is NOT event
        TSM_ASSERT( message, !(std::dynamic_pointer_cast<EventWorkspace>(work_out1)) );
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
    if (work_out1->isRaggedWorkspace()){
      TSM_ASSERT_LESS_THAN( message, 0, work_out1->y(0).size());
    } else {
      TSM_ASSERT_LESS_THAN( message, 0, work_out1->blocksize());
    }
    TSM_ASSERT_EQUALS( message, work_in1->getNumberHistograms(), work_out1->getNumberHistograms());

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
          break;
      }
    }
    else
    {
      // ------ Use expected answer --------------------
      bool breakOut=false;
      std::string  dummy;
      for (size_t wi=0; wi < work_out1->getNumberHistograms(); wi++) {
        const auto &xIn = work_in1->x(wi);
        const auto &xOut = work_out1->x(wi);
        const auto &yOut = work_out1->y(wi);
        const auto &eOut = work_out1->e(wi);
        for (size_t i=0; i < yOut.size(); i++) {
          TS_ASSERT_DELTA(xIn[i], xOut[i], 0.0001);
          const double sig3 = yOut[i];
          const double err3 = eOut[i];
          TS_ASSERT_DELTA(sig3, expectedValue, 0.0001);
          TS_ASSERT_DELTA(err3, expectedError, 0.0001);
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
    double sig1 = work_in1->y(i/work_in1->blocksize())[i%work_in1->blocksize()];
    double sig2 = work_in2->y(ws2Index/work_in2->blocksize())[ws2Index%work_in2->blocksize()];
    double sig3 = work_out1->y(i/work_in1->blocksize())[i%work_in1->blocksize()];

    TS_ASSERT_DELTA(work_in1->x(i/work_in1->blocksize())[i%work_in1->blocksize()],
        work_out1->x(i/work_in1->blocksize())[i%work_in1->blocksize()], 0.0001);

    double err1 = work_in1->e(i/work_in1->blocksize())[i%work_in1->blocksize()];
    double err2 = work_in2->e(ws2Index/work_in2->blocksize())[ws2Index%work_in2->blocksize()];
    double err3 = work_out1->e(i/work_in1->blocksize())[i%work_in1->blocksize()];

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
    std::set<int64_t> masking;
    masking.insert(0);
    masking.insert(2);
    masking.insert(7);

    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace123(nHist,nBins, 0, masking);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create2DWorkspace154(nHist,nBins, 0, masking);
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

    MatrixWorkspace_sptr output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("work_in1");
    TS_ASSERT(output);

    const auto &spectrumInfo = output->spectrumInfo();
    for( int i = 0; i < nHist; ++i )
    {
      TS_ASSERT(spectrumInfo.hasDetectors(i));
      if( masking.count(i) == 0 )
      {
        TS_ASSERT_EQUALS(spectrumInfo.isMasked(i), false);
      }
      else
      {
        TS_ASSERT_EQUALS(spectrumInfo.isMasked(i), true);
        double yValue = output->y(i)[0];
        TS_ASSERT_EQUALS(yValue, yValue );
        TS_ASSERT( !std::isinf(yValue) );
      }
    }
    AnalysisDataService::Instance().remove(lhs);
    AnalysisDataService::Instance().remove(rhs);
    if( !replaceInput ) AnalysisDataService::Instance().remove(outputSpace);
  }

  MatrixWorkspace_sptr create_RaggedWorkspace()
  {
    // create workspace with 2 histograms
    MatrixWorkspace_sptr raggedWS = WorkspaceCreationHelper::create2DWorkspace(2, 1);

    // create and replace histograms with ragged ones
    Mantid::MantidVec x_data{100., 200., 300., 400.};
    Mantid::MantidVec y_data{2., 2., 2.};
    Mantid::MantidVec e_data{2., 2., 2.};
    Mantid::HistogramData::HistogramBuilder builder;
    builder.setX(x_data);
    builder.setY(y_data);
    builder.setE(e_data);
    raggedWS->setHistogram(0, builder.build());

    Mantid::MantidVec x_data2{200., 400., 600.};
    Mantid::MantidVec y_data2{2., 2.};
    Mantid::MantidVec e_data2{2., 2.};
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
    auto result = performTest(lhs, rhs, false, DO_DIVIDE ? 1.0 : 4.0, DO_DIVIDE ? 1.4142135625 : 5.6568542436);
    TS_ASSERT(result->isRaggedWorkspace());
    TS_ASSERT_EQUALS(result->isDistribution(), DO_DIVIDE ? true : false);
    TS_ASSERT(result->YUnit().empty());
  }

  void test_RaggedWorkspace_sameunit()
  {
    auto lhs = create_RaggedWorkspace();
    auto rhs = create_RaggedWorkspace();
    lhs->setYUnit("counts");
    rhs->setYUnit("counts");
    DO_DIVIDE = true;
    auto result = performTest(lhs, rhs, false, DO_DIVIDE ? 1.0 : 4.0, DO_DIVIDE ? 1.4142135625 : 5.6568542436);
    TS_ASSERT(result->isRaggedWorkspace());
    TS_ASSERT_EQUALS(result->isDistribution(), DO_DIVIDE ? true : false);
    TS_ASSERT(result->YUnit().empty());
  }

  void test_RaggedWorkspace_and_single_value()
  {
    auto lhs = create_RaggedWorkspace();
    auto rhs = WorkspaceCreationHelper::createWorkspaceSingleValue(2);
    auto result = performTest(lhs, rhs, false, DO_DIVIDE ? 1.0 : 4.0, DO_DIVIDE ? 1.2247448711 : 4.8989794899);
    TS_ASSERT(result->isRaggedWorkspace());
  }

  void test_RaggedWorkspace_not_compatible_x()
  {
    auto lhs = create_RaggedWorkspace();
    auto rhs = WorkspaceCreationHelper::create2DWorkspace(2, 4);
    performTest_fails(lhs, rhs);
  }



};

//============================================================================
/** Performance test with large workspaces. */

class @MULTIPLYDIVIDETEST_CLASS@Performance : public CxxTest::TestSuite
{
  Workspace2D_sptr m_ws2D_1, m_ws2D_2;

public:
  static @MULTIPLYDIVIDETEST_CLASS@Performance *createSuite() { return new @MULTIPLYDIVIDETEST_CLASS@Performance(); }
  static void destroySuite( @MULTIPLYDIVIDETEST_CLASS@Performance *suite ) { delete suite; }

  void setUp() override
  {
    constexpr int histograms{100000};
    constexpr int bins{1000};
    m_ws2D_1 = WorkspaceCreationHelper::create2DWorkspace(histograms, bins);
    m_ws2D_2 = WorkspaceCreationHelper::create2DWorkspace(histograms, bins);
  }

  void test_large_2D()
  {
    constexpr bool doDivide{@MULTIPLYDIVIDETEST_DO_DIVIDE@};
    if (doDivide) {
      MatrixWorkspace_sptr out = m_ws2D_1 / m_ws2D_2;
    } else {
      MatrixWorkspace_sptr out = m_ws2D_1 * m_ws2D_2;
    }
  }
};
