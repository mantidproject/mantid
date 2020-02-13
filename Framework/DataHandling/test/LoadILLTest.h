// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADILLTEST_H_
#define MANTID_DATAHANDLING_LOADILLTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/Load.h"
#include "MantidKernel/ConfigService.h"

using Mantid::API::AnalysisDataService;
using Mantid::API::MatrixWorkspace_const_sptr;
using Mantid::DataHandling::Load;
using Mantid::Kernel::ConfigService;

class LoadILLTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLTest *createSuite() { return new LoadILLTest(); }
  static void destroySuite(LoadILLTest *suite) { delete suite; }

  void setUp() override {
    ConfigService::Instance().appendDataSearchSubDir("ILL/D11/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/D22/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/D33/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/D20/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/D2B/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/IN4/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/IN5/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/IN6/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/PANTHER/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/D17/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/Figaro/");

    ConfigService::Instance().setFacility("ILL");
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void checkLoader(std::string filename, std::string resultLoader) {
    Load alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename))
    TS_ASSERT_EQUALS(alg.getPropertyValue("LoaderName"), resultLoader)
  }

  void test_LoadSANS_D11() {
    std::string filename = "010560.nxs";
    std::string result = "LoadILLSANS";

    checkLoader(filename, result);
  }

  void test_LoadSANS_D33() {
    std::string filename = "002294.nxs";
    std::string result = "LoadILLSANS";

    checkLoader(filename, result);
  }

  void test_LoadSANS_D22() {
    std::string filename = "192068.nxs";
    std::string result = "LoadILLSANS";

    checkLoader(filename, result);
  }

  void test_LoadDiffraction_D2B() {
    std::string filename = "535401.nxs";
    std::string result = "LoadILLDiffraction";

    checkLoader(filename, result);
  }

  void test_LoadDiffraction_D20() {
    std::string filename = "967100.nxs";
    std::string result = "LoadILLDiffraction";

    checkLoader(filename, result);
  }

  void test_loadIndirect_IN16B() {
    std::string filename = "ILLIN16B_127500.nxs";
    std::string result = "LoadILLIndirect";

    checkLoader(filename, result);
  }

  void test_loadTOF_IN4() {
    std::string filename = "084446.nxs";
    std::string result = "LoadILLTOF";

    checkLoader(filename, result);
  }

  void test_loadTOF_IN5() {
    std::string filename = "104007.nxs";
    std::string result = "LoadILLTOF";

    checkLoader(filename, result);
  }

  void test_loadTOF_IN6() {
    std::string filename = "164192.nxs";
    std::string result = "LoadILLTOF";

    checkLoader(filename, result);
  }

  void test_loadTOF_PANTHER() {
    std::string filename = "001036.nxs";
    std::string result = "LoadILLTOF";

    checkLoader(filename, result);
  }

  void test_loadReflectometry_D17() {
    std::string filename = "317370.nxs";
    std::string result = "LoadILLReflectometry";

    checkLoader(filename, result);
  }

  void test_loadReflectometry_FIGARO() {
    std::string filename = "000002.nxs";
    std::string result = "LoadILLReflectometry";

    checkLoader(filename, result);
  }
};

#endif /* MANTID_DATAHANDLING_LOADILLTEST_H_ */
