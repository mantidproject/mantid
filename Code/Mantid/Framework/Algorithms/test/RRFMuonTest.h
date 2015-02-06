#ifndef MANTID_ALGORITHMS_RRFMUON_H_
#define MANTID_ALGORITHMS_RRFMUON_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/RRFMuon.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/Workspace.h"
#include <cmath>
#include <Poco/File.h>
#include <stdexcept>

using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

class RRFMuonTest : public CxxTest::TestSuite
{
public:

  void testName()
  {
    TS_ASSERT_EQUALS( rrfMuon.name(), "RRFMuon" );
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( rrfMuon.category(), "Muon" )
  }

  void testRRFMuonZeroFrequency()
  {
    // Test of the algorithm at zero frequency
    // At zero frequency input and output workspaces should contain the same X, Y data

    // Create input workspace with three spectra
    MatrixWorkspace_sptr ws = createDummyWorkspace();

    // Initialise
    TS_ASSERT_THROWS_NOTHING( rrfMuon.initialize() );
    TS_ASSERT( rrfMuon.isInitialized() );
    // Set Values
    TS_ASSERT_THROWS_NOTHING( rrfMuon.setProperty("InputWorkspace", ws) );
    TS_ASSERT_THROWS_NOTHING( rrfMuon.setProperty("OutputWorkspace", "outputWs") );
    TS_ASSERT_THROWS_NOTHING( rrfMuon.setProperty("Frequency", "0") );
    TS_ASSERT_THROWS_NOTHING( rrfMuon.setProperty("Phase", "0") );
    // Execute
    TS_ASSERT_THROWS_NOTHING(rrfMuon.execute());
    TS_ASSERT(rrfMuon.isExecuted());
    // Get result
    MatrixWorkspace_const_sptr ows =
      boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("outputWs"));
    TS_ASSERT(ows);

    // Checks
    // X values
    TS_ASSERT_EQUALS( ws->readX(0), ows->readX(0) );
    TS_ASSERT_EQUALS( ws->readX(1), ows->readX(1) );
    // Y values
    TS_ASSERT_EQUALS( ws->readY(0), ows->readY(0) );
    TS_ASSERT_EQUALS( ws->readY(1), ows->readY(1) );
  }

    void testRRFMuonNonZeroFrequency()
  {
    // Test of the algorithm at non-zero frequency

    // Create input workspace with three spectra
    MatrixWorkspace_sptr ws = createDummyWorkspace();

    // Initialise
    TS_ASSERT_THROWS_NOTHING( rrfMuon.initialize() );
    TS_ASSERT( rrfMuon.isInitialized() );
    // Set Values
    TS_ASSERT_THROWS_NOTHING( rrfMuon.setProperty("InputWorkspace", ws) );
    TS_ASSERT_THROWS_NOTHING( rrfMuon.setProperty("OutputWorkspace", "outputWs") );
    TS_ASSERT_THROWS_NOTHING( rrfMuon.setProperty("Frequency", "1") );
    TS_ASSERT_THROWS_NOTHING( rrfMuon.setProperty("Phase", "0") );
    // Execute
    TS_ASSERT_THROWS_NOTHING(rrfMuon.execute());
    TS_ASSERT(rrfMuon.isExecuted());
    // Get result
    MatrixWorkspace_const_sptr ows =
      boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("outputWs"));
    TS_ASSERT(ows);

    // Checks
    // X values
    TS_ASSERT_EQUALS( ws->readX(0), ows->readX(0) );
    TS_ASSERT_EQUALS( ws->readX(1), ows->readX(1) );
    // Y values
    // The input frequency is close to the precession frequency, so:
    // The real part of the RRF polarization should be close to 1 for all X values
    // The imaginary part should be close to 0 for all X values
    TS_ASSERT_DELTA( ows->readY(0)[  0], 1, 0.001 );
    TS_ASSERT_DELTA( ows->readY(0)[100], 1, 0.001 );
    TS_ASSERT_DELTA( ows->readY(0)[200], 1, 0.001 );
    TS_ASSERT_DELTA( ows->readY(1)[  0], 0, 0.001 );
    TS_ASSERT_DELTA( ows->readY(1)[100], 0, 0.001 );
    TS_ASSERT_DELTA( ows->readY(1)[200], 0, 0.001 );
  }

private:
  RRFMuon rrfMuon;

  MatrixWorkspace_sptr createDummyWorkspace()
  {
    int nBins = 300;
    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", 2, nBins+1, nBins);

    for (int i=0; i<nBins; i++)
    {
      double x = i/static_cast<int>(nBins);
      ws->dataX(0)[i] = x;
      ws->dataY(0)[i] = cos(2*M_PI*x);
      ws->dataX(1)[i] = x;
      ws->dataY(1)[i] = sin(2*M_PI*x);
    }

    ws->dataX(0)[nBins] = nBins;
    ws->dataX(1)[nBins] = nBins;

    return ws;
  }

};

#endif /* MANTID_ALGORITHMS_RRFMUON_H_ */
