#ifndef MANTID_ALGORITHMS_PHASEQUADMUONTEST_H_
#define MANTID_ALGORITHMS_PHASEQUADMUONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/LoadMuonNexus2.h"
#include "MantidAlgorithms/PhaseQuadMuon.h"
#include <Poco/File.h>
#include <stdexcept>

using namespace Mantid::Algorithms;
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

  void testExec()
  {
    loader.initialize();
    loader.setPropertyValue("Filename", "emu00006473.nxs");
    loader.setPropertyValue("OutputWorkspace", "EMU6473");
    TS_ASSERT_THROWS_NOTHING( loader.execute() );
    TS_ASSERT_EQUALS(loader.isExecuted(),true);

    MatrixWorkspace_sptr inputWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("EMU6473");
    
    std::string filename("TestPhaseTable.txt");
    generatePhaseTable(filename);

    TS_ASSERT_THROWS_NOTHING( phaseQuadMuon.setProperty("PhaseTable", "TestPhaseTable.txt") );
    TS_ASSERT_THROWS_NOTHING( phaseQuadMuon.setProperty("InputWorkspace", "EMU6473") );
    TS_ASSERT_THROWS_NOTHING( phaseQuadMuon.setProperty("OutputWorkspace", "EMU6473_out") );

    TS_ASSERT_THROWS_NOTHING( phaseQuadMuon.execute() );
    TS_ASSERT( phaseQuadMuon.isExecuted() );

    MatrixWorkspace_sptr outputWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("EMU6473_out");

    TS_ASSERT_EQUALS( outputWs->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS( outputWs->getSpectrum(0)->readX(), inputWs->getSpectrum(0)->readX() ); // Check outputWs X values
    TS_ASSERT_EQUALS( outputWs->getSpectrum(1)->readX(), inputWs->getSpectrum(1)->readX() );

    // TODO add more tests

    AnalysisDataService::Instance().remove("EMU6473"); // remove inputWs
    AnalysisDataService::Instance().remove("EMU6473_out"); // remove OutputWs
    Poco::File("TestPhaseTable.txt").remove(); // remove phase table
  }

  void generatePhaseTable (std::string filename)
  {
    std::ofstream ofile;
    ofile.open(filename.c_str());

    if (ofile.is_open())
    {
      // Write header
      ofile << "MuSR 64 det 12705-12715" << std::endl;
      ofile << "Top row of numbers are:" << std::endl;
      ofile << "#histos, typ. first good bin#, typ. bin# when pulse over, mean lag." << std::endl;
      ofile << "Tabulated numbers are, per histogram:" << std::endl;
      ofile << "det ok, asymmetry, phase, lag, deadtime_c, deadtime_m." << std::endl;
      ofile << "32 2 0 0" << std::endl;
      // Write data
      for (int i=0; i<16; i++)
      {
        ofile << "1 50.0 0.00 0.0 0.0 1" << std::endl;
        ofile << "1 50.0 1.57 0.0 0.0 1" << std::endl;
      }
      ofile.close();
    }
    else
    {
      throw std::runtime_error("Unable to open file to write.");
    }
  }

private:
  PhaseQuadMuon phaseQuadMuon;
  Mantid::DataHandling::LoadMuonNexus2 loader;
};

#endif /* MANTID_ALGORITHMS_PHASEQUADMUONTEST_H_ */