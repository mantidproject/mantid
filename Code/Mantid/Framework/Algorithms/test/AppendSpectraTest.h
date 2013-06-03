#ifndef MANTID_ALGORITHMS_AppendSpectraTEST_H_
#define MANTID_ALGORITHMS_AppendSpectraTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/AppendSpectra.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

class AppendSpectraTest : public CxxTest::TestSuite
{

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AppendSpectraTest *createSuite() { return new AppendSpectraTest(); }
  static void destroySuite( AppendSpectraTest *suite ) { delete suite; }

  AppendSpectraTest() :
    ws1Name("ConjoinWorkspacesTest_grp1"), ws2Name("ConjoinWorkspacesTest_grp2")
  {
  }


  void setupWS()
  {
    IAlgorithm* loader;
    loader = new Mantid::DataHandling::LoadRaw3;
    loader->initialize();
    loader->setPropertyValue("Filename", "OSI11886.raw");
    loader->setPropertyValue("OutputWorkspace", "top");
    loader->setPropertyValue("SpectrumMin","1");
    loader->setPropertyValue("SpectrumMax","10");
    TS_ASSERT_THROWS_NOTHING( loader->execute() );
    TS_ASSERT( loader->isExecuted() );
    delete loader;

    loader = new Mantid::DataHandling::LoadRaw3;
    loader->initialize();
    loader->setPropertyValue("Filename", "OSI11886.raw");
    loader->setPropertyValue("OutputWorkspace", "bottom");
    loader->setPropertyValue("SpectrumMin","11");
    loader->setPropertyValue("SpectrumMax","25");
    TS_ASSERT_THROWS_NOTHING( loader->execute() );
    TS_ASSERT( loader->isExecuted() );
    delete loader;
  }


  //----------------------------------------------------------------------------------------------
  void testExec()
  {
    setupWS();

    AppendSpectra alg;
    if ( !alg.isInitialized() ) alg.initialize();

    // Get the two input workspaces for later
    MatrixWorkspace_sptr in1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("top");
    MatrixWorkspace_sptr in2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("bottom");

    // Mask a spectrum and check it is carried over
    const size_t maskTop(5), maskBottom(10);
    in1->maskWorkspaceIndex(maskTop);
    in2->maskWorkspaceIndex(maskBottom);

    // Now it should succeed
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace1","top") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace2","bottom") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace","top") );
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("top") );
    TS_ASSERT_EQUALS( output->getNumberHistograms(), 25 );
    // Check a few values
    TS_ASSERT_EQUALS( output->readX(0)[0], in1->readX(0)[0] );
    TS_ASSERT_EQUALS( output->readX(15)[444], in2->readX(5)[444] );
    TS_ASSERT_EQUALS( output->readY(3)[99], in1->readY(3)[99] );
    TS_ASSERT_EQUALS( output->readE(7)[700], in1->readE(7)[700] );
    TS_ASSERT_EQUALS( output->readY(19)[55], in2->readY(9)[55] );
    TS_ASSERT_EQUALS( output->readE(10)[321], in2->readE(0)[321] );
    TS_ASSERT_EQUALS( output->getAxis(1)->spectraNo(5), in1->getAxis(1)->spectraNo(5) );
    TS_ASSERT_EQUALS( output->getAxis(1)->spectraNo(12), in2->getAxis(1)->spectraNo(2) );

    //Check masking
    TS_ASSERT_EQUALS(output->getDetector(maskTop)->isMasked(), true);
    TS_ASSERT_EQUALS(output->getDetector(10 + maskBottom)->isMasked(), true);
  }

  //----------------------------------------------------------------------------------------------
  void testExecMismatchedWorkspaces()
  {
    MatrixWorkspace_sptr ews = WorkspaceCreationHelper::CreateEventWorkspace(10, 10);

    // Check it fails if mixing event workspaces and workspace 2Ds
    AppendSpectra alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace1",ews) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace2",WorkspaceCreationHelper::Create2DWorkspace(10, 10)) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace","outevent") );
    alg.execute();
    TS_ASSERT( ! alg.isExecuted() );
  }


  //----------------------------------------------------------------------------------------------
  void performTest(bool event)
  {
    MatrixWorkspace_sptr ws1, ws2, out;
    int numBins = 20;

    if (event)
    {
      ws1 = WorkspaceCreationHelper::CreateEventWorkspace2(10, numBins); //2 events per bin
      ws2 = WorkspaceCreationHelper::CreateEventWorkspace2(5, numBins);
    }
    else
    {
      ws1 = WorkspaceCreationHelper::Create2DWorkspace(10, numBins);
      ws2 = WorkspaceCreationHelper::Create2DWorkspace(5, numBins);
    }
    AnalysisDataService::Instance().addOrReplace(ws1Name, ws1);
    AnalysisDataService::Instance().addOrReplace(ws2Name, ws2);

    AppendSpectra alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace1",ws1Name) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace2",ws2Name) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace",ws1Name) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() );

    TS_ASSERT_THROWS_NOTHING( out = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(ws1Name) ); )
    TS_ASSERT(out);
    if (!out) return;

    TS_ASSERT_EQUALS( out->getNumberHistograms(), 15);
    TS_ASSERT_EQUALS( out->blocksize(), numBins);

    for(size_t wi=0; wi<out->getNumberHistograms(); wi++)
    {
      TS_ASSERT_EQUALS( out->getSpectrum(wi)->getSpectrumNo(), specid_t(wi) );
      TS_ASSERT( !out->getSpectrum(wi)->getDetectorIDs().empty() );
      for(size_t x=0; x<out->blocksize(); x++)
        TS_ASSERT_DELTA(out->readY(wi)[x], 2.0, 1e-5);
    }
  }

  void test_events()
  {
    performTest(true);
  }

  void test_2D()
  {
    performTest(false);
  }

private:
  const std::string ws1Name;
  const std::string ws2Name;
};


#endif /* MANTID_ALGORITHMS_AppendSpectraTEST_H_ */
