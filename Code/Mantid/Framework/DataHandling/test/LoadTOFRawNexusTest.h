#ifndef LOADTOFRAWNEXUSTEST_H_
#define LOADTOFRAWNEXUSTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/LoadTOFRawNexus.h"
#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/AlgorithmHelper.h"

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

  void testExec()
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
    TS_ASSERT_EQUALS( spec->getSpectrumNo(), 2);
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
    TS_ASSERT_EQUALS( spec->getSpectrumNo(), 36540);
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
  void test_compare_to_event()
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

};

#endif /*LOADTOFRAWNEXUSTEST_H_*/
