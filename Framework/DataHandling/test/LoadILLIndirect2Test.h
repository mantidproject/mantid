// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/LoadILLIndirect2.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidTypes/Core/DateAndTimeHelpers.h"

using namespace Mantid::API;
using Mantid::DataHandling::LoadILLIndirect2;

class LoadILLIndirect2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLIndirect2Test *createSuite() { return new LoadILLIndirect2Test(); }
  static void destroySuite(LoadILLIndirect2Test *suite) { delete suite; }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_Init() {
    LoadILLIndirect2 loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT(loader.isInitialized())
  }

  void test_Name() {
    LoadILLIndirect2 loader;
    TS_ASSERT_EQUALS(loader.name(), "LoadILLIndirect");
  }

  void test_Version() {
    LoadILLIndirect2 loader;
    TS_ASSERT_EQUALS(loader.version(), 2);
  }

  void test_Load_2013_Format() {
    doExecTest(m_dataFile2013, 2057, 1024); // all single detectors
  }

  void test_Load_2015_Format() {
    doExecTest(m_dataFile2015, 2051); // only 2 out of 8 single detectors
  }

  void test_Confidence_2013_Format() { doConfidenceTest(m_dataFile2013); }

  void test_Confidence_2015_Format() { doConfidenceTest(m_dataFile2015); }

  void doConfidenceTest(const std::string &file) {
    LoadILLIndirect2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setPropertyValue("Filename", file);
    Mantid::Kernel::NexusDescriptor descr(alg.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS(alg.confidence(descr), 80);
  }

  void test_bats() { doExecTest(m_batsFile); }

  void test_first_tube_33() {
    LoadILLIndirect2 loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT(loader.isInitialized())
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", m_bats33degree));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("OutputWorkspace", "__out_ws"));
    TS_ASSERT_THROWS_NOTHING(loader.execute(););
    TS_ASSERT(loader.isExecuted());
    MatrixWorkspace_sptr output2D = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("__out_ws");
    const Mantid::API::Run &runlogs = output2D->run();
    TS_ASSERT(runlogs.hasProperty("PSD.PSD angle 1"));
    TS_ASSERT_DELTA(runlogs.getLogAsSingleValue("PSD.PSD angle 1"), 33.1, 0.01);
    const auto &detInfo = output2D->detectorInfo();
    constexpr double degToRad = M_PI / 180.;
    TS_ASSERT_DELTA(detInfo.twoTheta(65), 33.1 * degToRad, 0.01)
    checkTimeFormat(output2D);
  }

  void test_first_tube_251() {
    LoadILLIndirect2 loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT(loader.isInitialized())
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", m_firstTube251));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("OutputWorkspace", "__out_ws"));
    TS_ASSERT_THROWS_NOTHING(loader.execute(););
    TS_ASSERT(loader.isExecuted());
    MatrixWorkspace_sptr output2D = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("__out_ws");
    const Mantid::API::Run &runlogs = output2D->run();
    TS_ASSERT(runlogs.hasProperty("PSD.PSD angle 1"));
    TS_ASSERT_DELTA(runlogs.getLogAsSingleValue("PSD.PSD angle 1"), 25.1, 0.01);
    const auto &detInfo = output2D->detectorInfo();
    constexpr double degToRad = M_PI / 180.;
    TS_ASSERT_DELTA(detInfo.twoTheta(65), 25.1 * degToRad, 0.01);
    const std::string idf = output2D->getInstrument()->getFilename();
    TS_ASSERT_EQUALS(output2D->getInstrument()->getName(), "IN16BF");
    TS_ASSERT(idf.ends_with("IN16BF_Definition.xml"));
    checkTimeFormat(output2D);
  }

  void test_diffraction_bats() {
    // checks loading IN16B diffraction data acquired in bats mode with the data
    // written in the older way in the Nexus
    LoadILLIndirect2 loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT(loader.isInitialized())
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", m_batsDiffraction));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("OutputWorkspace", "__out_ws"));
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("LoadDetectors", "Diffractometer"));
    TS_ASSERT_THROWS_NOTHING(loader.execute(););
    TS_ASSERT(loader.isExecuted());
    MatrixWorkspace_sptr output2D = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("__out_ws");
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 2049)
    TS_ASSERT_EQUALS(output2D->blocksize(), 2048)

    // check some values near the center tubes to verify the geometry
    // used is from the older version
    TS_ASSERT_EQUALS(output2D->dataY(1050)[1156], 16)
    TS_ASSERT_EQUALS(output2D->dataY(871)[1157], 17)
    TS_ASSERT_EQUALS(output2D->dataY(746)[1157], 18)
    checkTimeFormat(output2D);
  }
  void test_diffraction_doppler() {
    // checks loading IN16B diffration data acquired in Doppler mode with the
    // data written in the newer way in the Nexus
    LoadILLIndirect2 loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT(loader.isInitialized())
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", m_dopplerDiffraction));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("OutputWorkspace", "__out_ws"));
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("LoadDetectors", "Diffractometer"));
    TS_ASSERT_THROWS_NOTHING(loader.execute(););
    TS_ASSERT(loader.isExecuted());
    MatrixWorkspace_sptr output2D = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("__out_ws");
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 2049)
    TS_ASSERT_EQUALS(output2D->blocksize(), 1024)

    // check some values near the center tubes to verify the geometry
    // used is from the newer version
    TS_ASSERT_EQUALS(output2D->dataY(1050)[558], 2)
    TS_ASSERT_EQUALS(output2D->dataY(873)[557], 2)
    TS_ASSERT_EQUALS(output2D->dataY(724)[561], 3)
    checkTimeFormat(output2D);
  }

  void doExecTest(const std::string &file, int numHist = 2051, int numChannels = 2048) {
    // Name of the output workspace.
    std::string outWSName("LoadILLIndirectTest_OutputWS");

    LoadILLIndirect2 loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT(loader.isInitialized())
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", file));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(loader.execute(););
    TS_ASSERT(loader.isExecuted());

    MatrixWorkspace_sptr output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    TS_ASSERT(output);

    MatrixWorkspace_sptr output2D = std::dynamic_pointer_cast<MatrixWorkspace>(output);
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), numHist);
    TS_ASSERT_EQUALS(output2D->blocksize(), numChannels);

    const Mantid::API::Run &runlogs = output->run();
    TS_ASSERT(runlogs.hasProperty("Facility"));
    TS_ASSERT_EQUALS(runlogs.getProperty("Facility")->value(), "ILL");
    checkTimeFormat(output);
  }

  void test_spectrometer_noSDdetectors() {
    // checks loading IN16B spectrometer data when no SD detectors were enabled
    LoadILLIndirect2 loader;
    loader.setChild(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT(loader.isInitialized())
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", "ILL/IN16B/353970.nxs"));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("OutputWorkspace", "__unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("LoadDetectors", "Spectrometer"));
    TS_ASSERT_THROWS_NOTHING(loader.execute(););
    TS_ASSERT(loader.isExecuted());
    MatrixWorkspace_sptr outputWS =
        std::shared_ptr<Mantid::API::MatrixWorkspace>(loader.getProperty("OutputWorkspace"));
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 2049)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 2048)

    TS_ASSERT_EQUALS(outputWS->dataY(58)[3], 1.0)
  }

  void checkTimeFormat(MatrixWorkspace_const_sptr outputWS) {
    TS_ASSERT(outputWS->run().hasProperty("start_time"));
    TS_ASSERT(
        Mantid::Types::Core::DateAndTimeHelpers::stringIsISO8601(outputWS->run().getProperty("start_time")->value()));
  }

private:
  std::string m_dataFile2013{"ILL/IN16B/034745.nxs"};
  std::string m_dataFile2015{"ILL/IN16B/127500.nxs"};
  std::string m_batsFile{"ILL/IN16B/215962.nxs"};
  std::string m_bats33degree{"ILL/IN16B/247933.nxs"};
  std::string m_firstTube251{"ILL/IN16B/136558.nxs"};
  std::string m_batsDiffraction{"ILL/IN16B/249290.nxs"};
  std::string m_dopplerDiffraction{"ILL/IN16B/276047.nxs"};
};

class LoadILLIndirect2TestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    for (int i = 0; i < numberOfIterations; ++i) {
      loadAlgPtrs.emplace_back(setupAlg());
    }
  }

  void testLoadILLIndirectPerformance() {
    for (auto alg : loadAlgPtrs) {
      TS_ASSERT_THROWS_NOTHING(alg->execute());
    }
  }

  void tearDown() override {
    for (int i = 0; i < numberOfIterations; i++) {
      delete loadAlgPtrs[i];
      loadAlgPtrs[i] = nullptr;
    }
    Mantid::API::AnalysisDataService::Instance().remove(outWSName);
  }

private:
  std::vector<LoadILLIndirect2 *> loadAlgPtrs;

  const int numberOfIterations = 5;

  const std::string inFileName = "ILL/IN16B/215962.nxs";
  const std::string outWSName = "LoadILLWsOut";

  LoadILLIndirect2 *setupAlg() {
    LoadILLIndirect2 *loader = new LoadILLIndirect2;
    loader->initialize();
    loader->isInitialized();
    loader->setPropertyValue("Filename", inFileName);
    loader->setPropertyValue("OutputWorkspace", outWSName);

    loader->setRethrows(true);
    return loader;
  }
};
