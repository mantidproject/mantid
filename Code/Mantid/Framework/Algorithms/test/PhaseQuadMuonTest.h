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

  void testExecPhaseList()
  {
    loader.initialize();
    loader.setPropertyValue("Filename", "emu00006473.nxs");
    loader.setPropertyValue("OutputWorkspace", "EMU6473");
    TS_ASSERT_THROWS_NOTHING( loader.execute() );
    TS_ASSERT_EQUALS(loader.isExecuted(),true);

    MatrixWorkspace_sptr inputWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("EMU6473");
    
    std::string filename("TestPhaseList.txt");
    generatePhaseList(filename);

    TS_ASSERT_THROWS_NOTHING( phaseQuadMuon.setProperty("PhaseList", "TestPhaseList.txt") );
    TS_ASSERT_THROWS_NOTHING( phaseQuadMuon.setProperty("InputWorkspace", "EMU6473") );
    TS_ASSERT_THROWS_NOTHING( phaseQuadMuon.setProperty("OutputWorkspace", "EMU6473_out") );

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

    // Check real spectrum Y values
    TS_ASSERT_DELTA ( specReY[ 0], -0.998265, 0.000001 );
    TS_ASSERT_DELTA ( specReY[10], -0.997286, 0.000001 );
    TS_ASSERT_DELTA ( specReY[20], -0.026196, 0.000001 );
    TS_ASSERT_DELTA ( specReY[30],  0.017798, 0.000001 );
    TS_ASSERT_DELTA ( specReY[40],  0.033196, 0.000001 );
    TS_ASSERT_DELTA ( specReY[50],  0.025337, 0.000001 );
    // Check real spectrum E values
    TS_ASSERT_DELTA ( specReE[ 0], 135268, 1 );
    TS_ASSERT_DELTA ( specReE[10], 145487, 1 );
    TS_ASSERT_DELTA ( specReE[20], 0.00213851, 0.000001 );
    TS_ASSERT_DELTA ( specReE[30], 0.00226644, 0.000001 );
    TS_ASSERT_DELTA ( specReE[40], 0.00237071, 0.000001 );
    TS_ASSERT_DELTA ( specReE[50], 0.00244977, 0.000001 );
    // Check imaginary spectrum Y values
    TS_ASSERT_DELTA ( specImY[ 0], -0.997455, 0.000001 );
    TS_ASSERT_DELTA ( specImY[10], -0.993110, 0.000001 );
    TS_ASSERT_DELTA ( specImY[20], 0.0099704, 0.000001 );
    TS_ASSERT_DELTA ( specImY[30], 0.0300842, 0.000001 );
    TS_ASSERT_DELTA ( specImY[40], 0.0285628, 0.000001 );
    TS_ASSERT_DELTA ( specImY[50], 0.0300885, 0.000001 );
    // Check imaginary spectrum E values
    TS_ASSERT_DELTA ( specImE[ 0], 280312, 1 );
    TS_ASSERT_DELTA ( specImE[10], 301487, 1 );
    TS_ASSERT_DELTA ( specImE[20], 0.00316581, 0.000001 );
    TS_ASSERT_DELTA ( specImE[30], 0.00332145, 0.000001 );
    TS_ASSERT_DELTA ( specImE[40], 0.00343792, 0.000001 );
    TS_ASSERT_DELTA ( specImE[50], 0.00357113, 0.000001 );

    AnalysisDataService::Instance().remove("EMU6473"); // remove inputWs
    AnalysisDataService::Instance().remove("EMU6473_out"); // remove OutputWs
    Poco::File("TestPhaseList.txt").remove(); // remove phase list
  }

  void generatePhaseList (std::string filename)
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
    TS_ASSERT_THROWS_NOTHING( phaseQuadMuon.setProperty("PhaseTable", "PhaseTable") );
    TS_ASSERT_THROWS_NOTHING( phaseQuadMuon.setProperty("PulseOver", "60") );
    TS_ASSERT_THROWS_NOTHING( phaseQuadMuon.setProperty("MeanLag", "0") );

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
    // Check real spectrum Y values
    TS_ASSERT_DELTA ( specReY[ 0], -0.998265, 0.000001 );
    TS_ASSERT_DELTA ( specReY[10], -0.997286, 0.000001 );
    TS_ASSERT_DELTA ( specReY[20], -0.026196, 0.000001 );
    TS_ASSERT_DELTA ( specReY[30],  0.017798, 0.000001 );
    TS_ASSERT_DELTA ( specReY[40],  0.033196, 0.000001 );
    TS_ASSERT_DELTA ( specReY[50],  0.025337, 0.000001 );
    // Check real spectrum E values
    TS_ASSERT_DELTA ( specReE[ 0], 135268, 1 );
    TS_ASSERT_DELTA ( specReE[10], 145487, 1 );
    TS_ASSERT_DELTA ( specReE[20], 0.00213851, 0.000001 );
    TS_ASSERT_DELTA ( specReE[30], 0.00226644, 0.000001 );
    TS_ASSERT_DELTA ( specReE[40], 0.00237071, 0.000001 );
    TS_ASSERT_DELTA ( specReE[50], 0.00244977, 0.000001 );
    // Check imaginary spectrum Y values
    TS_ASSERT_DELTA ( specImY[ 0], -0.997455, 0.000001 );
    TS_ASSERT_DELTA ( specImY[10], -0.993110, 0.000001 );
    TS_ASSERT_DELTA ( specImY[20], 0.0099704, 0.000001 );
    TS_ASSERT_DELTA ( specImY[30], 0.0300842, 0.000001 );
    TS_ASSERT_DELTA ( specImY[40], 0.0285628, 0.000001 );
    TS_ASSERT_DELTA ( specImY[50], 0.0300885, 0.000001 );
    // Check imaginary spectrum E values
    TS_ASSERT_DELTA ( specImE[ 0], 280312, 1 );
    TS_ASSERT_DELTA ( specImE[10], 301487, 1 );
    TS_ASSERT_DELTA ( specImE[20], 0.00316581, 0.000001 );
    TS_ASSERT_DELTA ( specImE[30], 0.00332145, 0.000001 );
    TS_ASSERT_DELTA ( specImE[40], 0.00343792, 0.000001 );
    TS_ASSERT_DELTA ( specImE[50], 0.00357113, 0.000001 );

    AnalysisDataService::Instance().remove("EMU6473"); // remove inputWs
    AnalysisDataService::Instance().remove("EMU6473_out"); // remove OutputWs
    AnalysisDataService::Instance().remove("PhaseTable"); // remove PhaseTable

  }

  void generatePhaseTable (ITableWorkspace_sptr phaseTable)
  {
    phaseTable->addColumn("bool","DetectorOK");
    phaseTable->addColumn("double","DetectorAlpha");
    phaseTable->addColumn("double","DetectorPhase");
    phaseTable->addColumn("double","DetectorDeadTime");
    for (int i=0; i<16; i++)
    {
      TableRow phaseRow1 = phaseTable->appendRow();
      phaseRow1 << true << 50.0 <<  0.0 << 0.0 ;
      TableRow phaseRow2 = phaseTable->appendRow();
      phaseRow2 << true << 50.0 << 1.57 << 0.0 ;
    }
  }

private:
  PhaseQuadMuon phaseQuadMuon;
  Mantid::DataHandling::LoadMuonNexus2 loader;
};

#endif /* MANTID_ALGORITHMS_PHASEQUADMUONTEST_H_ */