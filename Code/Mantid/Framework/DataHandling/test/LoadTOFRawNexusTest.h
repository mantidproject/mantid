#ifndef LOADTOFRAWNEXUSTEST_H_
#define LOADTOFRAWNEXUSTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/LoadTOFRawNexus.h"
#include "MantidTestHelpers/AlgorithmHelper.h"
#include <cxxtest/TestSuite.h>
#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadTOFRawNexusTest: public CxxTest::TestSuite
{
public:

  void testInit()
  {
    LoadTOFRawNexus alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );
  }

  void xtestExec()
  {
    Mantid::API::FrameworkManager::Instance();
    Mantid::DataHandling::LoadTOFRawNexus ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "CNCS_7860.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    Mantid::API::MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("outWS"));
    );
    TS_ASSERT(ws); if (!ws) return;
    TS_ASSERT_EQUALS(ws->blocksize(), 201);
    TS_ASSERT_EQUALS(ws->getInstrument()->getName(), "CNCS");
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 51200);

    ISpectrum * spec = ws->getSpectrum(2);
    TS_ASSERT_EQUALS( spec->getSpectrumNo(), 3);
    TS_ASSERT_EQUALS( spec->getDetectorIDs().size(), 1);
    TS_ASSERT( spec->hasDetectorID(2) );
    MantidVec X, Y, E;
    X = spec->dataX();
    Y = spec->dataY();
    E = spec->dataE();
    TS_ASSERT_EQUALS( X.size(), 202);
    TS_ASSERT_EQUALS( Y.size(), 201);
    TS_ASSERT_EQUALS( E.size(), 201);

    TS_ASSERT_DELTA( X[0], 43000, 1e-4);
    TS_ASSERT_DELTA( X[201], 63001, 1e-4);

    // Data is pretty sparse, look for a place with something
    TS_ASSERT_DELTA( Y[47], 1.0, 1e-4);
    TS_ASSERT_DELTA( E[47], 1.0, 1e-4);

    // More data in this spectrum
    spec = ws->getSpectrum(36540);
    TS_ASSERT_EQUALS( spec->getSpectrumNo(), 36541);
    TS_ASSERT_EQUALS( spec->getDetectorIDs().size(), 1);
    TS_ASSERT( spec->hasDetectorID(36540) );
    TS_ASSERT_DELTA( spec->dataY()[95], 133.0, 1e-4);
    TS_ASSERT_DELTA( spec->dataE()[95], sqrt(133.0), 1e-4);

    TS_ASSERT_EQUALS( ws->getAxis(1)->length(), 51200);
    TS_ASSERT_EQUALS( ws->getAxis(0)->length(), 202);
    TS_ASSERT_EQUALS( ws->getAxis(0)->unit()->caption(), "Time-of-flight");
    TS_ASSERT_EQUALS( ws->YUnit(), "Counts");
    TS_ASSERT_EQUALS( ws->getTitle(), "test after manual intervention");
  }

  /** Compare to LoadEventNexus
   * DISABLED because the order of spectra is different. */
  void xtest_compare_to_event()
  {
    AlgorithmHelper::runAlgorithm("LoadTOFRawNexus", 4,
        "Filename", "CNCS_7860.nxs",
        "OutputWorkspace", "outWS");

    AlgorithmHelper::runAlgorithm("LoadEventNexus", 4,
        "Filename", "CNCS_7860_event.nxs",
        "OutputWorkspace", "outWS_event");

    // Convert to 2D
    AlgorithmHelper::runAlgorithm("Rebin", 8,
        "InputWorkspace", "outWS_event",
        "Params", "43000, 100, 63000, 1, 63001",
        "OutputWorkspace", "outWS_event_2D",
        "PreserveEvents", "0");

    // Compare workspaces
    Mantid::API::Algorithm_sptr alg = AlgorithmHelper::runAlgorithm(
        "CheckWorkspacesMatch", 8,
        "Workspace1", "outWS",
        "Workspace2", "outWS_event_2D",
        "Tolerance", "1e-4",
        "CheckAxes", "0");
    // We skip Axis check because of floating point imprecision makes a false negative.

    std::string s = alg->getPropertyValue("Result");
    TS_ASSERT_EQUALS( s, "Success!");

    Mantid::API::MatrixWorkspace_sptr ws1, ws2;
    TS_ASSERT_THROWS_NOTHING(
        ws1 = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("outWS"));
    );
    TS_ASSERT_THROWS_NOTHING(
        ws2 = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("outWS_event_2D"));
    );
    TS_ASSERT(ws1); if (!ws1) return;
    TS_ASSERT(ws2); if (!ws2) return;

    // Quick axes check.
    TS_ASSERT_EQUALS( ws1->getAxis(0)->length(), ws2->getAxis(0)->length())
    TS_ASSERT_EQUALS( ws1->getAxis(1)->length(), ws2->getAxis(1)->length())
  }

  void test_bad_signal_fails()
  {
    Mantid::API::Algorithm_sptr alg;
    // Number points to a 2D data set
    alg = AlgorithmHelper::runAlgorithm("LoadTOFRawNexus", 6,
        "Filename", "CNCS_7860.nxs", "Signal", "2", "OutputWorkspace", "outWS");
    TS_ASSERT( !alg->isExecuted() );

    // Number is too big
    alg = AlgorithmHelper::runAlgorithm("LoadTOFRawNexus", 6,
        "Filename", "CNCS_7860.nxs", "Signal", "6", "OutputWorkspace", "outWS");
    TS_ASSERT( !alg->isExecuted() );

  }

  /** Refs #3716: Different signals (binned in q-space, d-space, tof)
   * File is rather large (and slow to load) so not in SVN.
   * Test passes if file is not found.
   *
   * @param signal :: signal number to load
   * @param expectedXLength :: # of bins
   * */
  Mantid::API::MatrixWorkspace_sptr do_test_signal(int signal, size_t expectedXLength)
  {
    Mantid::API::AnalysisDataService::Instance().remove("outWS");
    MemoryManager::Instance().releaseFreeMemory();
    std::string filename = "NOM_2011_09_15T16_17_30Z_histo.nxs";
    Mantid::API::FrameworkManager::Instance();
    Mantid::DataHandling::LoadTOFRawNexus ld;
    ld.initialize();
    try{ ld.setPropertyValue("Filename", filename); }
    catch (...)
    { std::cout << "Test not completed due to missing data file " << filename << std::endl;
      return Mantid::API::MatrixWorkspace_sptr();
    }
    ld.setProperty("Signal", signal);
    ld.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    Mantid::API::MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("outWS"));
    );
    TS_ASSERT(ws);
    if (!ws) return ws;
    TS_ASSERT_EQUALS( ws->getAxis(0)->length(), expectedXLength);
    TS_ASSERT_EQUALS( ws->blocksize(), expectedXLength-1);
    TS_ASSERT_EQUALS( ws->getNumberHistograms(), 99*8*128);
    return ws;
  }


  void test_signal_1()
  {
    Mantid::API::MatrixWorkspace_sptr ws = do_test_signal(1, 168);
    if (!ws) return;
    TS_ASSERT_DELTA( ws->getAxis(0)->operator ()(0,0), 0, 1e-6);
    TS_ASSERT_DELTA( ws->getAxis(0)->operator ()(1,0), 1000, 1e-6);
    TS_ASSERT_EQUALS( ws->getAxis(0)->unit()->unitID(), "TOF");
  }

  void test_signal_5()
  {
    Mantid::API::MatrixWorkspace_sptr ws = do_test_signal(5, 2501);
    if (!ws) return;
    TS_ASSERT_DELTA( ws->getAxis(0)->operator ()(0,0), 0.02, 1e-6);
    TS_ASSERT_DELTA( ws->getAxis(0)->operator ()(1,0), 0.04, 1e-6);
    TS_ASSERT_EQUALS( ws->getAxis(0)->unit()->unitID(), "MomentumTransfer");
  }

  void test_signal_6()
  {
    Mantid::API::MatrixWorkspace_sptr ws = do_test_signal(6, 2521);
    if (!ws) return;
    TS_ASSERT_DELTA( ws->getAxis(0)->operator ()(0,0), 0.125, 1e-6);
    TS_ASSERT_DELTA( ws->getAxis(0)->operator ()(1,0), 0.250, 1e-6);
    TS_ASSERT_EQUALS( ws->getAxis(0)->unit()->unitID(), "dSpacing");
  }


};

#endif /*LOADTOFRAWNEXUSTEST_H_*/
