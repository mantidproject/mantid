#ifndef MANTID_ALGORITHMS_RRFMUON_H_
#define MANTID_ALGORITHMS_RRFMUON_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/RRFMuon.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid::Algorithms;
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
    TS_ASSERT_THROWS_NOTHING( rrfMuon.setProperty("Frequency units", "MHz") );
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
    TS_ASSERT_THROWS_NOTHING( rrfMuon.setProperty("Frequency units", "MHz") );
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

    void testRRFMuonUnits()
  {
    // Test of the algorithm at non-zero frequency

    // Create input workspace with three spectra
    MatrixWorkspace_sptr ws = createDummyWorkspace();

    // Initialise
    TS_ASSERT_THROWS_NOTHING( rrfMuon.initialize() );
    TS_ASSERT_THROWS_NOTHING( rrfMuon2.initialize() );
    TS_ASSERT_THROWS_NOTHING( rrfMuon3.initialize() );
    TS_ASSERT( rrfMuon.isInitialized() );
    TS_ASSERT( rrfMuon2.isInitialized() );
    TS_ASSERT( rrfMuon3.isInitialized() );
    // Set Values
    // First run
    TS_ASSERT_THROWS_NOTHING( rrfMuon.setProperty("InputWorkspace", ws) );
    TS_ASSERT_THROWS_NOTHING( rrfMuon.setProperty("OutputWorkspace", "outputWs1") );
    TS_ASSERT_THROWS_NOTHING( rrfMuon.setProperty("Frequency", "1") );
    TS_ASSERT_THROWS_NOTHING( rrfMuon.setProperty("Frequency units", "MHz") );
    TS_ASSERT_THROWS_NOTHING( rrfMuon.setProperty("Phase", "0") );
    // Second run
    TS_ASSERT_THROWS_NOTHING( rrfMuon2.setProperty("InputWorkspace", ws) );
    TS_ASSERT_THROWS_NOTHING( rrfMuon2.setProperty("OutputWorkspace", "outputWs2") );
    TS_ASSERT_THROWS_NOTHING( rrfMuon2.setProperty("Frequency", "0.159155") );
    TS_ASSERT_THROWS_NOTHING( rrfMuon2.setProperty("Frequency units", "Mrad/s") );
    TS_ASSERT_THROWS_NOTHING( rrfMuon2.setProperty("Phase", "0") );
    // Third run
    TS_ASSERT_THROWS_NOTHING( rrfMuon3.setProperty("InputWorkspace", ws) );
    TS_ASSERT_THROWS_NOTHING( rrfMuon3.setProperty("OutputWorkspace", "outputWs3") );
    TS_ASSERT_THROWS_NOTHING( rrfMuon3.setProperty("Frequency", "11.742398") );
    TS_ASSERT_THROWS_NOTHING( rrfMuon3.setProperty("Frequency units", "Gauss") );
    TS_ASSERT_THROWS_NOTHING( rrfMuon3.setProperty("Phase", "0") );
    // Execute all of them
    TS_ASSERT_THROWS_NOTHING(rrfMuon.execute());
    TS_ASSERT_THROWS_NOTHING(rrfMuon2.execute());
    TS_ASSERT_THROWS_NOTHING(rrfMuon3.execute());
    TS_ASSERT(rrfMuon.isExecuted());
    TS_ASSERT(rrfMuon2.isExecuted());
    TS_ASSERT(rrfMuon3.isExecuted());
    // Get results
    MatrixWorkspace_const_sptr ows1 =
      boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("outputWs1"));
    TS_ASSERT(ows1);
    MatrixWorkspace_const_sptr ows2 =
      boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("outputWs2"));
    TS_ASSERT(ows2);
    MatrixWorkspace_const_sptr ows3 =
      boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("outputWs3"));
    TS_ASSERT(ows3);

    // Check Y values
    // ows1 vs ows2
    // Results with different frequency units should be very similar
    TS_ASSERT_DELTA  ( ows1->readY(0)[  5], ows2->readY(0)[  5], 0.000001 );
    TS_ASSERT_DELTA  ( ows1->readY(0)[ 98], ows2->readY(0)[ 98], 0.000001 );
    TS_ASSERT_DELTA  ( ows1->readY(0)[276], ows2->readY(0)[276], 0.000001 );
    // But not exactly the same
    // (They should only be the same if the input frequency in rrfMuon2 were exactly 1/2/M_PI)
    TS_ASSERT_DIFFERS( ows1->readY(0)[  5], ows2->readY(0)[  5]);
    TS_ASSERT_DIFFERS( ows1->readY(0)[ 98], ows2->readY(0)[ 98]);
    TS_ASSERT_DIFFERS( ows1->readY(0)[276], ows2->readY(0)[276]);
    // ows1 vs ows3
    // Results with different frequency units should be very similar
    TS_ASSERT_DELTA  ( ows1->readY(0)[  8], ows3->readY(0)[  8], 0.000001 );
    TS_ASSERT_DELTA  ( ows1->readY(0)[109], ows3->readY(0)[109], 0.000001 );
    TS_ASSERT_DELTA  ( ows1->readY(0)[281], ows3->readY(0)[281], 0.000001 );
    // But not exactly the same
    // (They should only be the same if the input frequency in rrfMuon3 were exactly 1/2/M_PI/MU
    // being MU the muon gyromagnetic ratio)
    TS_ASSERT_DIFFERS( ows1->readY(0)[  8], ows3->readY(0)[  8]);
    TS_ASSERT_DIFFERS( ows1->readY(0)[109], ows3->readY(0)[109]);
    TS_ASSERT_DIFFERS( ows1->readY(0)[281], ows3->readY(0)[281]);
  }

private:
  RRFMuon rrfMuon;
  RRFMuon rrfMuon2, rrfMuon3;

  MatrixWorkspace_sptr createDummyWorkspace()
  {
    int nBins = 300;
    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", 2, nBins+1, nBins);

    for (int i=0; i<nBins; i++)
    {
      double x = i/static_cast<double>(nBins);
      ws->dataX(0)[i] = x;
      ws->dataY(0)[i] = cos(2*M_PI*x);
      ws->dataX(1)[i] = x;
      ws->dataY(1)[i] = sin(2*M_PI*x);
    }

    ws->dataX(0)[nBins] = nBins;
    ws->dataX(1)[nBins] = nBins;

    // Units
    ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");

    return ws;
  }

};

#endif /* MANTID_ALGORITHMS_RRFMUON_H_ */
