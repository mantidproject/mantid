#ifndef LOADTOFRAWNEXUSTEST_H_
#define LOADTOFRAWNEXUSTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/LoadTOFRawNexus.h"
#include <cxxtest/TestSuite.h>
#include <Poco/File.h>
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MemoryManager.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using Mantid::MantidVec;

class LoadTOFRawNexusTest: public CxxTest::TestSuite
{
public:

  void test_init()
  {
    LoadTOFRawNexus alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );
  }

  void test_confidence()
  {
    LoadTOFRawNexus alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );

    alg.setPropertyValue("Filename", "REF_L_32035.nxs");
    Mantid::Kernel::HDFDescriptor descr(alg.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS( alg.confidence(descr), 80 );

    alg.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    Mantid::Kernel::HDFDescriptor descr2(alg.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS( alg.confidence(descr2), 20 );

    alg.setPropertyValue("Filename", "argus0026577.nxs");
    Mantid::Kernel::HDFDescriptor descr3(alg.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS( alg.confidence(descr3), 0 );

    alg.setPropertyValue("Filename", "PG3_733.nxs");
    Mantid::Kernel::HDFDescriptor descr4(alg.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS( alg.confidence(descr4), 0 );

  }

  void test_exec()
  {
    Mantid::DataHandling::LoadTOFRawNexus ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "REF_L_32035.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    Mantid::API::MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("outWS"));
    );
    TS_ASSERT(ws); if (!ws) return;
    TS_ASSERT_EQUALS(ws->blocksize(), 501);
    TS_ASSERT_EQUALS(ws->getInstrument()->getName(), "REF_L");
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 77824);

    ISpectrum * spec = ws->getSpectrum(27955);
    TS_ASSERT_EQUALS( spec->getSpectrumNo(), 27956);
    TS_ASSERT_EQUALS( spec->getDetectorIDs().size(), 1);
    TS_ASSERT( spec->hasDetectorID(27955) );
    MantidVec X, Y, E;
    X = spec->dataX();
    Y = spec->dataY();
    E = spec->dataE();
    TS_ASSERT_EQUALS( X.size(), 502);
    TS_ASSERT_EQUALS( Y.size(), 501);
    TS_ASSERT_EQUALS( E.size(), 501);

    TS_ASSERT_DELTA( X[0], 0, 1e-4);
    TS_ASSERT_DELTA( X[201], 40200, 1e-4);

    // Data is pretty sparse, look for a place with something
    TS_ASSERT_DELTA( Y[94], 1.0, 1e-4);
    TS_ASSERT_DELTA( E[94], 1.0, 1e-4);

    // More data in this spectrum
    spec = ws->getSpectrum(38019);
    TS_ASSERT_EQUALS( spec->getSpectrumNo(), 38020);
    TS_ASSERT_EQUALS( spec->getDetectorIDs().size(), 1);
    TS_ASSERT( spec->hasDetectorID(38019) );
    TS_ASSERT_DELTA( spec->dataY()[105], 23.0, 1e-4);
    TS_ASSERT_DELTA( spec->dataE()[105], sqrt(23.0), 1e-4);

    TS_ASSERT_EQUALS( ws->getAxis(1)->length(), 77824);
    TS_ASSERT_EQUALS( ws->getAxis(0)->length(), 502);
    TS_ASSERT_EQUALS( ws->getAxis(0)->unit()->caption(), "Time-of-flight");
    TS_ASSERT_EQUALS( ws->YUnit(), "Counts");
    TS_ASSERT_EQUALS( ws->getTitle(), "JAA-I-103B2-1_No4Rep0");
  }

  /** Compare to LoadEventNexus */
  void xtest_compare_to_event() ///< DISABLED because it takes ~ 4 seconds.
  {
    FrameworkManager::Instance().exec("LoadTOFRawNexus", 4,
        "Filename", "CNCS_7860.nxs",
        "OutputWorkspace", "outWS");

    FrameworkManager::Instance().exec("LoadEventNexus", 4,
        "Filename", "CNCS_7860_event.nxs",
        "OutputWorkspace", "outWS_event");

    // Convert to 2D
    FrameworkManager::Instance().exec("Rebin", 8,
        "InputWorkspace", "outWS_event",
        "Params", "43000, 100, 63000, 1, 63001",
        "OutputWorkspace", "outWS_event_2D",
        "PreserveEvents", "0");

    // Compare workspaces
    Mantid::API::IAlgorithm_sptr alg = FrameworkManager::Instance().exec(
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
    Mantid::API::IAlgorithm_sptr alg;
    // Number points to a 2D data set
    alg = FrameworkManager::Instance().exec("LoadTOFRawNexus", 6,
        "Filename", "REF_L_32035.nxs", "Signal", "2", "OutputWorkspace", "outWS");
    TS_ASSERT( !alg->isExecuted() );

    // Number is too big
    alg = FrameworkManager::Instance().exec("LoadTOFRawNexus", 6,
        "Filename", "REF_L_32035.nxs", "Signal", "6", "OutputWorkspace", "outWS");
    TS_ASSERT( !alg->isExecuted() );
  }

  void test_old_file()
  {
    // Just need to make sure that it runs
    Mantid::DataHandling::LoadTOFRawNexus ld;
    ld.initialize();
    ld.setProperty("Filename", "REF_L_7139.nxs");
    ld.setProperty("OutputWorkspace", "REF_L_7139");
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());
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

  /** Disabled because it is slow */
  void xtest_signal_6()
  {
    Mantid::API::MatrixWorkspace_sptr ws = do_test_signal(6, 2521);
    if (!ws) return;
    TS_ASSERT_DELTA( ws->getAxis(0)->operator ()(0,0), 0.125, 1e-6);
    TS_ASSERT_DELTA( ws->getAxis(0)->operator ()(1,0), 0.250, 1e-6);
    TS_ASSERT_EQUALS( ws->getAxis(0)->unit()->unitID(), "dSpacing");
  }

  class LoadTOFRawNexusExposed : public LoadTOFRawNexus
  {
  public:
    void doExec()
    {
      this->exec();
    }
  };

  void xtest_SNAP_3893() ///< DISABLED because it takes > 60 seconds.
  {
    LoadTOFRawNexusExposed alg;
    alg.initialize();
    alg.setPropertyValue("Filename","SNAP_3893.nxs");
    alg.setPropertyValue("OutputWorkspace","outWS");
    alg.doExec();
    TS_ASSERT( alg.isExecuted() );
  };
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadTOFRawNexusTestPerformance : public CxxTest::TestSuite
{
public:
  void testDefaultLoad()
  {
    LoadTOFRawNexus loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "REF_L_32035.nxs");
    loader.setPropertyValue("OutputWorkspace", "ws");
    TS_ASSERT( loader.execute() );
  }
};

#endif /*LOADTOFRAWNEXUSTEST_H_*/
