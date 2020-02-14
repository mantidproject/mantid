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
    ConfigService::Instance().appendDataSearchSubDir("ILL/IN16B/");
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

  void test_LoadSANS_D11() { checkLoader("010560.nxs", "LoadILLSANS"); }

  void test_LoadSANS_D33() {
    checkLoader("002294.nxs", "LoadILLSANS");
    checkLoader("042610.nxs", "LoadILLSANS"); // D33 TOF
  }

  void test_LoadSANS_D22() { checkLoader("192068.nxs", "LoadILLSANS"); }

  void test_LoadDiffraction_D2B() {
    checkLoader("535401.nxs", "LoadILLDiffraction");
  }

  void test_LoadDiffraction_D20() {
    checkLoader("967076.nxs", "LoadILLDiffraction");
    checkLoader("967087.nxs", "LoadILLDiffraction");
  }

  void test_loadIndirect_IN16B() {
    checkLoader("090661.nxs", "LoadILLIndirect");    // one wing qens
    checkLoader("083072.nxs", "LoadILLIndirect");    // one wing efws
    checkLoader("083073.nxs", "LoadILLIndirect");    // one wing ifws
    checkLoader("136558-136559", "LoadILLIndirect"); // two wings qens
    checkLoader("143720.nxs", "LoadILLIndirect");    // two wings efws
    checkLoader("170300.nxs", "LoadILLIndirect");    // two wings ifws
    checkLoader("215962.nxs", "LoadILLIndirect");    // bats
  }

  void test_loadTOF_IN4() { checkLoader("084446.nxs", "LoadILLTOF"); }

  void test_loadTOF_IN5() {
    checkLoader("104007.nxs", "LoadILLTOF");
    checkLoader("189171.nxs", "LoadILLTOF");
  }

  void test_loadTOF_IN6() {
    checkLoader("164192.nxs", "LoadILLTOF");
    checkLoader("220010.nxs", "LoadILLTOF");
  }

  void test_loadTOF_PANTHER() {
    checkLoader("001036.nxs", "LoadILLTOF");
    checkLoader("001723.nxs", "LoadILLTOF");
  }

  void test_loadReflectometry_D17() {
    checkLoader("317370.nxs", "LoadILLReflectometry");
  }

  void test_loadReflectometry_FIGARO() {
    checkLoader("000002.nxs", "LoadILLReflectometry");
  }
};

#endif /* MANTID_DATAHANDLING_LOADILLTEST_H_ */
