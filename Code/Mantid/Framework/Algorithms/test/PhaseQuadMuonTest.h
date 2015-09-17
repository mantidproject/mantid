#ifndef MANTID_ALGORITHMS_PHASEQUADMUONTEST_H_
#define MANTID_ALGORITHMS_PHASEQUADMUONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/LoadMuonNexus2.h"
#include "MantidAlgorithms/PhaseQuadMuon.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include <Poco/File.h>
#include <stdexcept>

using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

class PhaseQuadMuonTest : public CxxTest::TestSuite
{
public:

  void testName()
  {
    TS_ASSERT_EQUALS( phaseQuadMuon.name(), "PhaseQuad" );
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( phaseQuadMuon.category(), "Muon" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( phaseQuadMuon.initialize() );
    TS_ASSERT( phaseQuadMuon.isInitialized() )
  }

  void testExecPhaseTable()
  {
    loader.initialize();
    loader.setPropertyValue("Filename", "emu00006473.nxs");
    loader.setPropertyValue("OutputWorkspace", "EMU6473");
    TS_ASSERT_THROWS_NOTHING( loader.execute() );
    TS_ASSERT_EQUALS(loader.isExecuted(),true);

    MatrixWorkspace_sptr inputWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("EMU6473");

    boost::shared_ptr<ITableWorkspace>phaseTable(new Mantid::DataObjects::TableWorkspace);
    generatePhaseTable(phaseTable);
    AnalysisDataService::Instance().add("PhaseTable",phaseTable);

    TS_ASSERT_THROWS_NOTHING( phaseQuadMuon.setProperty("InputWorkspace", "EMU6473") );
    TS_ASSERT_THROWS_NOTHING( phaseQuadMuon.setProperty("OutputWorkspace", "EMU6473_out") );
    TS_ASSERT_THROWS_NOTHING( phaseQuadMuon.setProperty("DetectorTable", "PhaseTable") );

    TS_ASSERT_THROWS_NOTHING( phaseQuadMuon.execute() );
    TS_ASSERT( phaseQuadMuon.isExecuted() );

    MatrixWorkspace_sptr outputWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("EMU6473_out");

    TS_ASSERT_EQUALS( outputWs->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS( outputWs->getSpectrum(0)->readX(), inputWs->getSpectrum(0)->readX() ); // Check outputWs X values
    TS_ASSERT_EQUALS( outputWs->getSpectrum(1)->readX(), inputWs->getSpectrum(1)->readX() );

    auto specReY = outputWs->getSpectrum(0)->readY();
    auto specReE = outputWs->getSpectrum(0)->readE();
    auto specImY = outputWs->getSpectrum(1)->readY();
    auto specImE = outputWs->getSpectrum(1)->readE();
    // Check real Y values
    TS_ASSERT_DELTA ( specReY[ 0], -0.9984, 0.0001 );
    TS_ASSERT_DELTA ( specReY[20], -0.1316, 0.0001 );
    TS_ASSERT_DELTA ( specReY[50], -0.0856, 0.0001 );
    // Check real E values
    TS_ASSERT_DELTA ( specReE[ 0], 0.0019, 0.0001);
    TS_ASSERT_DELTA ( specReE[20], 0.0020, 0.0001 );
    TS_ASSERT_DELTA ( specReE[50], 0.0022, 0.0001 );
    // Check imaginary Y values
    TS_ASSERT_DELTA ( specImY[ 0], -0.9976, 0.0001 );
    TS_ASSERT_DELTA ( specImY[20], -0.0988, 0.0001 );
    TS_ASSERT_DELTA ( specImY[50], -0.0808, 0.0001 );
    // Check imaginary E values
    TS_ASSERT_DELTA ( specImE[ 0], 0.0027, 0.0001 );
    TS_ASSERT_DELTA ( specImE[20], 0.0029, 0.0001 );
    TS_ASSERT_DELTA ( specImE[50], 0.0033, 0.0001 );

    AnalysisDataService::Instance().remove("EMU6473"); // remove inputWs
    AnalysisDataService::Instance().remove("EMU6473_out"); // remove OutputWs
    AnalysisDataService::Instance().remove("PhaseTable"); // remove PhaseTable

  }

  void generatePhaseTable (ITableWorkspace_sptr phaseTable)
  {
    phaseTable->addColumn("int","DetectorID");
    phaseTable->addColumn("double","DetectorPhase");
    for (int i=0; i<16; i++)
    {
      TableRow phaseRow1 = phaseTable->appendRow();
      phaseRow1 << i << 0.0 ;
      TableRow phaseRow2 = phaseTable->appendRow();
      phaseRow2 << i << 1.57 ;
    }
  }

private:
  PhaseQuadMuon phaseQuadMuon;
  Mantid::DataHandling::LoadMuonNexus2 loader;
};

#endif /* MANTID_ALGORITHMS_PHASEQUADMUONTEST_H_ */