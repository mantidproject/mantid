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
    std::string instrumentPaths[12] = {
        "ILL/D11/",   "ILL/D22/",     "ILL/D33/", "ILL/D20/",
        "ILL/D2B/",   "ILL/IN4/",     "ILL/IN5/", "ILL/IN6/",
        "ILL/IN16B/", "ILL/PANTHER/", "ILL/D17/", "ILL/Figaro/"};

    for (size_t i = 0; i < 12; i++) {
      ConfigService::Instance().appendDataSearchSubDir(instrumentPaths[i]);
    }

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

  void test_LoadSANS_D11() { checkLoader("010560", "LoadILLSANS"); }

  void test_LoadSANS_D33() {
    checkLoader("002294", "LoadILLSANS");
    checkLoader("042610", "LoadILLSANS"); // D33 TOF
  }

  void test_LoadSANS_D22() { checkLoader("192068", "LoadILLSANS"); }

  void test_LoadDiffraction_D2B() {
    checkLoader("535401", "LoadILLDiffraction");
  }

  void test_LoadDiffraction_D20() {
    checkLoader("967076", "LoadILLDiffraction");
    checkLoader("967087", "LoadILLDiffraction");
  }

  void test_loadIndirect_IN16B() {
    checkLoader("090661", "LoadILLIndirect"); // one wing qens
    checkLoader("083072", "LoadILLIndirect"); // one wing efws
    checkLoader("083073", "LoadILLIndirect"); // one wing ifws
    checkLoader("136558", "LoadILLIndirect"); // two wings qens
    checkLoader("143720", "LoadILLIndirect"); // two wings efws
    checkLoader("170300", "LoadILLIndirect"); // two wings ifws
    checkLoader("215962", "LoadILLIndirect"); // bats
  }

  void test_loadTOF_IN4() { checkLoader("084446", "LoadILLTOF"); }

  void test_loadTOF_IN5() {
    checkLoader("104007", "LoadILLTOF");
    checkLoader("189171", "LoadILLTOF");
  }

  void test_loadTOF_IN6() {
    checkLoader("164192", "LoadILLTOF");
    checkLoader("220010", "LoadILLTOF");
  }

  void test_loadTOF_PANTHER() {
    checkLoader("001036", "LoadILLTOF");
    checkLoader("001723", "LoadILLTOF");
  }

  void test_loadReflectometry_D17() {
    checkLoader("317370", "LoadILLReflectometry");
  }

  void test_loadReflectometry_FIGARO() {
    checkLoader("000002", "LoadILLReflectometry");
  }
};

#endif /* MANTID_DATAHANDLING_LOADILLTEST_H_ */
