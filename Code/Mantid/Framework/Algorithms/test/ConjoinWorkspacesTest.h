#ifndef CONJOINWORKSPACESTEST_H_
#define CONJOINWORKSPACESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ConjoinWorkspaces.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class ConjoinWorkspacesTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConjoinWorkspacesTest *createSuite() { return new ConjoinWorkspacesTest(); }
  static void destroySuite( ConjoinWorkspacesTest *suite ) { delete suite; }

  ConjoinWorkspacesTest() :
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

  void testTheBasics()
  {
    ConjoinWorkspaces conj;
    TS_ASSERT_EQUALS( conj.name(), "ConjoinWorkspaces" );
    TS_ASSERT_EQUALS( conj.version(), 1 );
    TS_ASSERT_THROWS_NOTHING( conj.initialize() );
    TS_ASSERT( conj.isInitialized() );
  }

  //----------------------------------------------------------------------------------------------
  void testExec()
  {
    setupWS();

    ConjoinWorkspaces conj;
    if ( !conj.isInitialized() ) conj.initialize();

    // Get the two input workspaces for later
    MatrixWorkspace_sptr in1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("top"));
    MatrixWorkspace_sptr in2 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("bottom"));

    // Mask a spectrum and check it is carried over
    const size_t maskTop(5), maskBottom(10);
    in1->maskWorkspaceIndex(maskTop);
    in2->maskWorkspaceIndex(maskBottom);

    // Check it fails if properties haven't been set
    TS_ASSERT_THROWS( conj.execute(), std::runtime_error );
    TS_ASSERT( ! conj.isExecuted() );

    // Check it fails if input overlap
    TS_ASSERT_THROWS_NOTHING( conj.setPropertyValue("InputWorkspace1","top") );
    TS_ASSERT_THROWS_NOTHING( conj.setPropertyValue("InputWorkspace2","top") );
    TS_ASSERT_THROWS_NOTHING( conj.execute() );
    TS_ASSERT( ! conj.isExecuted() );

    // Now it should succeed
    TS_ASSERT_THROWS_NOTHING( conj.setPropertyValue("InputWorkspace1","top") );
    TS_ASSERT_THROWS_NOTHING( conj.setPropertyValue("InputWorkspace2","bottom") );
    TS_ASSERT_THROWS_NOTHING( conj.execute() );
    TS_ASSERT( conj.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("top")) );
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

    // Check that 2nd input workspace no longer exists
    TS_ASSERT_THROWS( AnalysisDataService::Instance().retrieve("bottom"), Exception::NotFoundError );

    // Check that th workspace has the correct number of history entries
    TS_ASSERT_EQUALS(output->getHistory().size(), 3);
  }

  //----------------------------------------------------------------------------------------------
  void testExecMismatchedWorkspaces()
  {
    MatrixWorkspace_sptr ews = WorkspaceCreationHelper::CreateEventWorkspace(10, 10);

    // Check it fails if input overlap
    ConjoinWorkspaces conj;
    conj.initialize();
    TS_ASSERT_THROWS_NOTHING( conj.setProperty("InputWorkspace1",ews) );
    TS_ASSERT_THROWS_NOTHING( conj.setProperty("InputWorkspace2",ews) );
    conj.execute();
    TS_ASSERT( ! conj.isExecuted() );

    // Check it fails if mixing event workspaces and workspace 2Ds
    TS_ASSERT_THROWS_NOTHING( conj.setProperty("InputWorkspace1",ews) );
    TS_ASSERT_THROWS_NOTHING( conj.setProperty("InputWorkspace2",WorkspaceCreationHelper::Create2DWorkspace(10, 10)) );
    conj.execute();
    TS_ASSERT( ! conj.isExecuted() );
  }


  void testDoCheckForOverlap()
  {
    MatrixWorkspace_sptr ws1, ws2;
    int numPixels = 10;
    int numBins = 20;
    ws1 = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins);
    const std::string ws1_name = "ConjoinWorkspaces_testDoCheckForOverlap";
    AnalysisDataService::Instance().add(ws1_name, ws1);
    ws2 = WorkspaceCreationHelper::CreateEventWorkspace(5, numBins);

    ConjoinWorkspaces conj;
    conj.initialize();
    TS_ASSERT_THROWS_NOTHING( conj.setPropertyValue("InputWorkspace1",ws1_name) );
    TS_ASSERT_THROWS_NOTHING( conj.setProperty("InputWorkspace2",ws2) );
    TS_ASSERT_THROWS_NOTHING( conj.setProperty("CheckOverlapping",true) );
    TS_ASSERT_THROWS_NOTHING(conj.execute());
    // Falls over as they overlap
    TS_ASSERT( !conj.isExecuted() );

    // Adjust second workspace
    Mantid::specid_t start = ws1->getSpectrum(numPixels - 1)->getSpectrumNo() + 10;
    for( int i = 0; i < 5; ++i)
    {
      Mantid::API::ISpectrum *spec = ws2->getSpectrum(i);
      spec->setSpectrumNo(start + i);
      spec->clearDetectorIDs();
      spec->addDetectorID(start + i);
    }

    TS_ASSERT_THROWS_NOTHING( conj.setProperty("InputWorkspace2",ws2) );
    TS_ASSERT_THROWS_NOTHING(conj.execute());
    TS_ASSERT( conj.isExecuted() );

    // Test output
    MatrixWorkspace_sptr output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(ws1_name));
    TS_ASSERT(output);
    // Check the first spectrum has the correct ID
    TS_ASSERT_EQUALS( output->getNumberHistograms(), 15 );
    TS_ASSERT_EQUALS( output->getSpectrum(0)->getSpectrumNo(), ws1->getSpectrum(0)->getSpectrumNo() );
    // and the joining point
    TS_ASSERT_EQUALS( output->getSpectrum(10)->getSpectrumNo(), start);
    TS_ASSERT( ! output->getSpectrum(11)->getDetectorIDs().empty() );

    AnalysisDataService::Instance().remove(ws1_name);
  }

  void performTestNoOverlap(bool event)
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

    ConjoinWorkspaces conj;
    conj.initialize();
    TS_ASSERT_THROWS_NOTHING( conj.setPropertyValue("InputWorkspace1",ws1Name) );
    TS_ASSERT_THROWS_NOTHING( conj.setPropertyValue("InputWorkspace2",ws2Name) );
    TS_ASSERT_THROWS_NOTHING( conj.setProperty("CheckOverlapping",false) );
    TS_ASSERT_THROWS_NOTHING( conj.execute(); )
    TS_ASSERT( conj.isExecuted() );

    TS_ASSERT_THROWS_NOTHING( out = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(ws1Name) ); )
    TS_ASSERT(out);
    if (!out) return;

    TS_ASSERT_EQUALS( out->getNumberHistograms(), 15);
    TS_ASSERT_EQUALS( out->blocksize(), numBins);

    for(size_t wi=0; wi<out->getNumberHistograms(); wi++)
      for(size_t x=0; x<out->blocksize(); x++)
        TS_ASSERT_DELTA(out->readY(wi)[x], 2.0, 1e-5);
  }

  void test_DONTCheckForOverlap_Events()
  {
    performTestNoOverlap(true);
  }

  void test_DONTCheckForOverlap_2D()
  {
    performTestNoOverlap(false);
  }

private:
  const std::string ws1Name;
  const std::string ws2Name;
};

#endif /*CONJOINWORKSPACESTEST_H_*/
