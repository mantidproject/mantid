// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/LoadILLSALSA.h"

class LoadILLSALSATest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLSALSATest *createSuite() { return new LoadILLSALSATest(); }
  static void destroySuite(LoadILLSALSATest *suite) { delete suite; }

  LoadILLSALSATest() {
    Mantid::Kernel::ConfigService::Instance().appendDataSearchSubDir("ILL/SALSA/");
    Mantid::Kernel::ConfigService::Instance().setFacility("ILL");
  }

  void tearDown() override { Mantid::API::AnalysisDataService::Instance().clear(); }

  void test_name() {
    Mantid::DataHandling::LoadILLSALSA alg;
    TS_ASSERT_EQUALS(alg.name(), "LoadILLSALSA");
  }

  void test_version() {
    Mantid::DataHandling::LoadILLSALSA alg;
    TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void test_init() {
    Mantid::DataHandling::LoadILLSALSA alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }
};

class LoadILLSALSATestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    m_alg.initialize();
    m_alg.setChild(true);
    m_alg.setPropertyValue("Filename", "ILL/SALSA/046430.nxs");
    m_alg.setPropertyValue("OutputWorkspace", "__unused_for_child");
  }
  void testLoadILLSALSAPerformance() {
    for (int i = 0; i < 10; ++i) {
      TS_ASSERT_THROWS_NOTHING(m_alg.execute());
    }
  }
  void tearDown() override { Mantid::API::AnalysisDataService::Instance().clear(); }

private:
  Mantid::DataHandling::LoadILLSALSA m_alg;
};
