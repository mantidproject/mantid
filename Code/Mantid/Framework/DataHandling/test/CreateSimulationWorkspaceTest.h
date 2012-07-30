#ifndef MANTID_DATAHANDLING_CREATESIMULATIONWORKSPACETEST_H_
#define MANTID_DATAHANDLING_CREATESIMULATIONWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/CreateSimulationWorkspace.h"

using Mantid::DataHandling::CreateSimulationWorkspace;

class CreateSimulationWorkspaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateSimulationWorkspaceTest *createSuite() { return new CreateSimulationWorkspaceTest(); }
  static void destroySuite( CreateSimulationWorkspaceTest *suite ) { delete suite; }

  CreateSimulationWorkspaceTest()
    : m_wsName("CreateSimulationWorkspaceTest")
  {
  }

  void test_Init()
  {
    Mantid::API::IAlgorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING(alg = createAlgorithm());
  }

  void tearDown()
  {
    using Mantid::API::AnalysisDataService;
    if(AnalysisDataService::Instance().doesExist(m_wsName))
    {
      AnalysisDataService::Instance().remove(m_wsName);
    }
  }
  
  void test_Execute_With_Unknown_Instrument_Throws()
  {
    using namespace Mantid::API;
    Mantid::API::IAlgorithm_sptr alg = createAlgorithm(m_wsName);
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("BinParams", "1,1,10"));

    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Instrument", "__NOT_AN_INSTRUMENT__"));
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
  }

  void test_Valid_Params_Gives_Workspace_Of_With_Right_Number_Bins_And_Same_No_Histograms_As_Detectors_Without_Monitors()
  {
    auto outputWS = runAlgorithm("HET");

    TS_ASSERT(outputWS);

    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 12120);
    const size_t expectedNBins = 103;
    TS_ASSERT_EQUALS(outputWS->readY(0).size(), expectedNBins);
    TS_ASSERT_EQUALS(outputWS->readE(0).size(), expectedNBins);

    doBinCheck(outputWS, expectedNBins+1);
    doInstrumentCheck(outputWS, "HET", 12120);
  }

  void test_Default_Spectra_Detector_Mapping_Is_One_To_One()
  {
    using namespace Mantid::API;
    auto outputWS = runAlgorithm("HET");

    doInstrumentCheck(outputWS, "HET", 12120);
    const size_t nhist = outputWS->getNumberHistograms();
    TS_ASSERT_EQUALS(nhist, 12120);

    for(size_t i = 0; i < nhist; ++i)
    {
      ISpectrum *spectrum(NULL);
      TS_ASSERT_THROWS_NOTHING(spectrum = outputWS->getSpectrum(i));

      TS_ASSERT_EQUALS(spectrum->getSpectrumNo(), i + 1);
      TS_ASSERT_EQUALS(spectrum->getDetectorIDs().size(), 1);
    }
  }

  void xtest_Spectra_Detector_Mapping_Is_Pulled_From_Given_RAW_File()
  {
    using namespace Mantid::API;
    auto outputWS = runAlgorithm("HET", "DeltaE", "HET15869.raw");

    doInstrumentCheck(outputWS, "HET", 12120);
    const size_t nhist = outputWS->getNumberHistograms();
    TS_ASSERT_EQUALS(nhist, 2529);

    TS_ASSERT_EQUALS( outputWS->getSpectrum(6)->getDetectorIDs().size(), 1);
    TS_ASSERT_EQUALS( outputWS->getSpectrum(6)->getSpectrumNo(), 7);
    TS_ASSERT_EQUALS( outputWS->getSpectrum(2083)->getDetectorIDs().size(), 10);
    TS_ASSERT_EQUALS( outputWS->getSpectrum(2083)->getSpectrumNo(), 2084);
  }

  void test_Spectra_Detector_Mapping_Is_Pulled_From_Given_ISIS_NeXus_File()
  {
    using namespace Mantid::API;
    auto outputWS = runAlgorithm("LOQ", "DeltaE", "LOQ49886.nxs");

    const size_t nhist = outputWS->getNumberHistograms();
    TS_ASSERT_EQUALS(nhist, 17790);

    TS_ASSERT_EQUALS( outputWS->getSpectrum(6)->getDetectorIDs().size(), 1);
    TS_ASSERT_EQUALS( outputWS->getSpectrum(6)->getSpectrumNo(), 7);
    TS_ASSERT_EQUALS( outputWS->getSpectrum(2083)->getDetectorIDs().size(), 1);
    TS_ASSERT_EQUALS( outputWS->getSpectrum(2083)->getSpectrumNo(), 2084);
  }

  void test_UnitX_Throws_When_Invalid()
  {
    auto alg = boost::make_shared<CreateSimulationWorkspace>();
    alg->initialize();

    TS_ASSERT_THROWS(alg->setPropertyValue("UnitX", "NOT_A_UNIT"), std::invalid_argument);
  }

  void test_UnitX_Parameter_Is_DeltaE_By_Default()
  {
    auto outputWS = runAlgorithm("HET");

    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), "DeltaE");
  }

  void test_UnitX_Parameter_Is_Respected()
  {
    const std::string unitx = "TOF";
    auto outputWS = runAlgorithm("HET", unitx);

    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), unitx);
  }

private:

  Mantid::API::MatrixWorkspace_sptr runAlgorithm(const std::string & inst, const std::string & unitx = "",
                                                 const std::string & maptable = "")
  {
    using namespace Mantid::API;
    Mantid::API::IAlgorithm_sptr alg = createAlgorithm(m_wsName);

    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Instrument", inst));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("BinParams", "-30,3,279"));
    if(!unitx.empty()) TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("UnitX", unitx));
    if(!maptable.empty()) TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("DetectorTableFilename", maptable));

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    return Mantid::API::AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_wsName);
  }

  void doBinCheck(Mantid::API::MatrixWorkspace_sptr outputWS, const size_t expectedSize)
  {
    TS_ASSERT_EQUALS(outputWS->readX(0).size(), expectedSize);
    // Check bins are correct
    const Mantid::MantidVec & bins = outputWS->readX(0);
    for(size_t i = 0; i < bins.size(); ++i)
    {
      const double expected = -30.0 + static_cast<double>(i)*3;
      TS_ASSERT_DELTA(bins[i], expected, 1e-10);
    }
  }

  void doInstrumentCheck(Mantid::API::MatrixWorkspace_sptr outputWS,
                         const std::string & name, const size_t ndets)
  {
    Mantid::Geometry::Instrument_const_sptr instr = outputWS->getInstrument();

    TS_ASSERT(instr);
    TS_ASSERT_EQUALS(instr->getName(), name);
    TS_ASSERT_EQUALS(instr->getNumberDetectors(true), ndets);
  }

  Mantid::API::IAlgorithm_sptr createAlgorithm(const std::string & wsName = "")
  {
    auto alg = boost::make_shared<CreateSimulationWorkspace>();
    alg->setRethrows(true);
    alg->initialize();
    if(!wsName.empty()) alg->setPropertyValue("OutputWorkspace", wsName);
    return alg;
  }  

  std::string m_wsName;
};


#endif /* MANTID_DATAHANDLING_CREATESIMULATIONWORKSPACETEST_H_ */
