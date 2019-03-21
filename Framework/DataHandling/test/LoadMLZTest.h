// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef LOADMLZTEST_H_
#define LOADMLZTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidDataHandling/LoadMLZ.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using Mantid::DataHandling::LoadMLZ;

class LoadMLZTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadMLZTest *createSuite() { return new LoadMLZTest(); }
  static void destroySuite(LoadMLZTest *suite) { delete suite; }

  LoadMLZTest() : m_dataFile("TOFTOFTestdata.nxs") {}

  void testName() {
    LoadMLZ loader;
    TS_ASSERT_EQUALS(loader.name(), "LoadMLZ");
  }

  void testVersion() {
    LoadMLZ loader;
    TS_ASSERT_EQUALS(loader.version(), 1);
  }

  void testInit() {
    LoadMLZ loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
  }

  /*
   * This test loading of the Sample Data
   */
  void testLoad() {
    LoadMLZ loader;
    loader.initialize();
    loader.setPropertyValue("Filename", m_dataFile);

    std::string outputSpace = "LoadMLZTest_out";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING(loader.execute());

    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputSpace);

    TS_ASSERT_EQUALS(output->getNumberHistograms(), 1006);

    // test whether instrument parameter Efixed has been set
    auto instrument = output->getInstrument();
    TS_ASSERT(instrument->hasParameter("Efixed"));
    auto efixed = instrument->getNumberParameter("Efixed")[0];
    TS_ASSERT_DELTA(efixed, 2.272, 0.001);

    AnalysisDataService::Instance().clear();
  }

private:
  std::string m_dataFile;
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadMLZTestPerformance : public CxxTest::TestSuite {
public:
  LoadMLZTestPerformance() : m_dataFile("TOFTOFTestdata.nxs") {}

  static LoadMLZTestPerformance *createSuite() {
    return new LoadMLZTestPerformance();
  }

  static void destroySuite(LoadMLZTestPerformance *suite) { delete suite; }

  void setUp() override {
    loader.initialize();
    loader.setPropertyValue("Filename", m_dataFile);
    loader.setPropertyValue("OutputWorkspace", "ws");
  }

  void tearDown() override { AnalysisDataService::Instance().remove("ws"); }

  void testDefaultLoad() { loader.execute(); }

private:
  Mantid::DataHandling::LoadMLZ loader;
  std::string m_dataFile;
};

#endif /*LoadMLZTEST_H_*/
