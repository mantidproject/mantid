// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/CreateSimulationWorkspace.h"
#include "MantidKernel/Unit.h"

using Mantid::DataHandling::CreateSimulationWorkspace;

class CreateSimulationWorkspaceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateSimulationWorkspaceTest *createSuite() { return new CreateSimulationWorkspaceTest(); }
  static void destroySuite(CreateSimulationWorkspaceTest *suite) { delete suite; }

  CreateSimulationWorkspaceTest() : m_wsName("CreateSimulationWorkspaceTest") {}

  void test_Init() {
    Mantid::API::IAlgorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING(alg = createAlgorithm());
  }

  void tearDown() override {
    using Mantid::API::AnalysisDataService;
    if (AnalysisDataService::Instance().doesExist(m_wsName)) {
      AnalysisDataService::Instance().remove(m_wsName);
    }
  }

  void test_Execute_With_Unknown_Instrument_Throws() {
    using namespace Mantid::API;
    auto alg = createAlgorithm(m_wsName);
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("BinParams", "1,1,10"));

    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Instrument", "__NOT_AN_INSTRUMENT__"));
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void
  test_Valid_Params_Gives_Workspace_Of_With_Right_Number_Bins_And_Same_No_Histograms_As_Detectors_Without_Monitors() {
    auto outputWS = runAlgorithm("HET");

    TS_ASSERT(outputWS);

    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 12120);
    const size_t expectedNBins = 103;
    TS_ASSERT_EQUALS(outputWS->readY(0).size(), expectedNBins);
    TS_ASSERT_EQUALS(outputWS->readE(0).size(), expectedNBins);

    doBinCheck(outputWS, expectedNBins + 1);
    doInstrumentCheck(outputWS, "HET", 12120);
  }

  void test_Default_Spectra_Detector_Mapping_Is_One_To_One() {
    using namespace Mantid::API;
    auto outputWS = runAlgorithm("HET");

    doInstrumentCheck(outputWS, "HET", 12120);
    const size_t nhist = outputWS->getNumberHistograms();
    TS_ASSERT_EQUALS(nhist, 12120);

    for (size_t i = 0; i < nhist; ++i) {
      TS_ASSERT_THROWS_NOTHING(outputWS->getSpectrum(i));
      auto &spectrum = outputWS->getSpectrum(i);

      TS_ASSERT_EQUALS(spectrum.getSpectrumNo(), i + 1);
      TS_ASSERT_EQUALS(spectrum.getDetectorIDs().size(), 1);
    }
  }

  void test_Spectra_Detector_Mapping_Is_Pulled_From_Given_RAW_File() {
    using namespace Mantid::API;
    std::string filename("HET15869.raw");
    auto outputWS = runAlgorithm("HET", "DeltaE", filename);

    doInstrumentCheck(outputWS, "HET", 12120);
    const size_t nhist = outputWS->getNumberHistograms();
    TS_ASSERT_EQUALS(nhist, 2529);

    TS_ASSERT_EQUALS(outputWS->getSpectrum(6).getDetectorIDs().size(), 1);
    TS_ASSERT_EQUALS(outputWS->getSpectrum(6).getSpectrumNo(), 7);
    TS_ASSERT_EQUALS(outputWS->getSpectrum(2083).getDetectorIDs().size(), 10);
    TS_ASSERT_EQUALS(outputWS->getSpectrum(2083).getSpectrumNo(), 2084);

    // The HET15869 data set was measured around 2005 on the HET instrument.
    // This is also the latest IDF.
    compareSimulationWorkspaceIDFWithFileIDF(outputWS, filename, "LoadRaw");
  }

  void test_correct_IDF_is_used_for_RAW_File() {
    using namespace Mantid::API;
    std::string filename("LOQ48094.raw");
    auto outputWS = runAlgorithm("LOQ", "DeltaE", filename);

    // doInstrumentCheck(outputWS, "LOQ", 12120);
    const size_t nhist = outputWS->getNumberHistograms();
    TS_ASSERT_EQUALS(nhist, 17790);

    // The LOQ48094 data set was measured around 2008 on the LOQ instrument.
    // This latest IDF is 2012 or later.
    compareSimulationWorkspaceIDFWithFileIDF(outputWS, filename, "LoadRaw");
  }

  void test_Spectra_Detector_Mapping_Is_Pulled_From_Given_ISIS_NeXus_File() {
    using namespace Mantid::API;
    std::string filename("LOQ49886.nxs");
    auto outputWS = runAlgorithm("LOQ", "DeltaE", filename);

    const size_t nhist = outputWS->getNumberHistograms();
    TS_ASSERT_EQUALS(nhist, 17790);

    TS_ASSERT_EQUALS(outputWS->getSpectrum(6).getDetectorIDs().size(), 1);
    TS_ASSERT_EQUALS(outputWS->getSpectrum(6).getSpectrumNo(), 7);
    TS_ASSERT_EQUALS(outputWS->getSpectrum(2083).getDetectorIDs().size(), 1);
    TS_ASSERT_EQUALS(outputWS->getSpectrum(2083).getSpectrumNo(), 2084);

    // The LOQ49886 data set was measured around 2009 on the LOQ instrument.
    // It does not link to the most recent version of the LOQ IDF (2012 or
    // later).
    compareSimulationWorkspaceIDFWithFileIDF(outputWS, filename, "LoadNexus");
  }

  void test_UnitX_Throws_When_Invalid() {
    auto alg = std::make_shared<CreateSimulationWorkspace>();
    alg->initialize();

    TS_ASSERT_THROWS(alg->setPropertyValue("UnitX", "NOT_A_UNIT"), const std::invalid_argument &);
  }

  void test_UnitX_Parameter_Is_DeltaE_By_Default() {
    auto outputWS = runAlgorithm("HET");

    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), "DeltaE");
  }

  void test_UnitX_Parameter_Is_Respected() {
    const std::string unitx = "TOF";
    auto outputWS = runAlgorithm("HET", unitx);

    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), unitx);
  }

  void test_Bin_Errors() {
    auto outputWS = runAlgorithm("HET");

    const Mantid::MantidVec &bins = outputWS->readE(0);
    for (size_t i = 0; i < bins.size(); ++i) {
      TS_ASSERT_DELTA(bins[i], sqrt(outputWS->readY(0)[i]), 1e-10);
    }

    // Re-run to verify errors are 0 when flag is unset
    outputWS = runAlgorithm("HET", "", "", false);
    const Mantid::MantidVec &binsNoErr = outputWS->readE(0);
    for (size_t i = 0; i < binsNoErr.size(); ++i) {
      TS_ASSERT_DELTA(binsNoErr[i], 0.0, 1e-10);
    }
  }

private:
  Mantid::API::MatrixWorkspace_sptr runAlgorithm(const std::string &inst, const std::string &unitx = "",
                                                 const std::string &maptable = "", bool setErrors = true) {
    using namespace Mantid::API;
    auto alg = createAlgorithm(m_wsName);

    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Instrument", inst));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("BinParams", "-30,3,279"));
    if (!unitx.empty())
      TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("UnitX", unitx));
    if (!maptable.empty())
      TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("DetectorTableFilename", maptable));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("SetErrors", setErrors));

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    return Mantid::API::AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_wsName);
  }

  void doBinCheck(const Mantid::API::MatrixWorkspace_sptr &outputWS, const size_t expectedSize) {
    TS_ASSERT_EQUALS(outputWS->readX(0).size(), expectedSize);
    // Check bins are correct
    const Mantid::MantidVec &bins = outputWS->readX(0);
    for (size_t i = 0; i < bins.size(); ++i) {
      const double expected = -30.0 + static_cast<double>(i) * 3;
      TS_ASSERT_DELTA(bins[i], expected, 1e-10);
    }
  }

  void doInstrumentCheck(const Mantid::API::MatrixWorkspace_sptr &outputWS, const std::string &name,
                         const size_t ndets) {
    Mantid::Geometry::Instrument_const_sptr instr = outputWS->getInstrument();

    TS_ASSERT(instr);
    TS_ASSERT_EQUALS(instr->getName(), name);
    TS_ASSERT_EQUALS(instr->getNumberDetectors(true), ndets);
  }

  Mantid::API::IAlgorithm_sptr createAlgorithm(const std::string &wsName = "") {
    auto alg = std::make_shared<CreateSimulationWorkspace>();
    alg->setRethrows(true);
    alg->initialize();
    if (!wsName.empty())
      alg->setPropertyValue("OutputWorkspace", wsName);
    return alg;
  }

  void compareSimulationWorkspaceIDFWithFileIDF(const Mantid::API::MatrixWorkspace_sptr &simulationWorkspace,
                                                const std::string &filename, const std::string &algorithmName) {
    std::string outputWSName = "outWSIDFCompareNexus";
    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(algorithmName);
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("Filename", filename);
    alg->setProperty("OutputWorkspace", outputWSName);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    Mantid::API::Workspace_sptr outWS = alg->getProperty("OutputWorkspace");
    auto matWS = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(outWS);
    auto idfForOriginal = matWS->getInstrument()->getFilename();
    auto idfForSimulationWS = simulationWorkspace->getInstrument()->getFilename();
    TSM_ASSERT_EQUALS("Should have the same IDF", idfForOriginal, idfForSimulationWS);
  }

  std::string m_wsName;
};

class CreateSimulationWorkspaceTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    alg.initialize();

    alg.setPropertyValue("Instrument", "HET");
    alg.setPropertyValue("BinParams", "-30,3,279");
    alg.setPropertyValue("OutputWorkspace", outWsName);

    alg.setRethrows(true);
  }

  void testCreateSimulationWorkspacePerformance() { TS_ASSERT_THROWS_NOTHING(alg.execute()); }

  void tearDown() override { Mantid::API::AnalysisDataService::Instance().remove(outWsName); }

private:
  CreateSimulationWorkspace alg;

  const std::string outWsName = "outTestWs";
};
