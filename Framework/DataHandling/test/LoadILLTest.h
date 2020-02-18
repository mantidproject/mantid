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

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void checkLoader(std::string filename, std::string resultLoader) {
    Load alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename))
    TS_ASSERT_EQUALS(alg.getPropertyValue("LoaderName"), resultLoader)
  }

  void test_LoadSANS_D11() { checkLoader("ILL/D11/010560", "LoadILLSANS"); }

  void test_LoadSANS_D33() {
    checkLoader("ILL/D33/002294", "LoadILLSANS");
    checkLoader("ILL/D33/042610", "LoadILLSANS"); // D33 TOF
  }

  void test_LoadSANS_D22() { checkLoader("ILL/D22/192068", "LoadILLSANS"); }

  void test_LoadDiffraction_D2B() {
    checkLoader("ILL/D2B/535401", "LoadILLDiffraction");
  }

  void test_LoadDiffraction_D20() {
    checkLoader("ILL/D20/967076", "LoadILLDiffraction");
    checkLoader("ILL/D20/967087", "LoadILLDiffraction");
  }

  void test_loadIndirect_IN16B() {
    checkLoader("ILL/IN16B/090661", "LoadILLIndirect"); // one wing qens
    checkLoader("ILL/IN16B/083072", "LoadILLIndirect"); // one wing efws
    checkLoader("ILL/IN16B/083073", "LoadILLIndirect"); // one wing ifws
    checkLoader("ILL/IN16B/136558", "LoadILLIndirect"); // two wings qens
    checkLoader("ILL/IN16B/143720", "LoadILLIndirect"); // two wings efws
    checkLoader("ILL/IN16B/170300", "LoadILLIndirect"); // two wings ifws
    checkLoader("ILL/IN16B/215962", "LoadILLIndirect"); // bats
  }

  void test_loadTOF_IN4() { checkLoader("ILL/IN4/084446", "LoadILLTOF"); }

  void test_loadTOF_IN5() {
    checkLoader("ILL/IN5/104007", "LoadILLTOF");
    checkLoader("ILL/IN5/189171", "LoadILLTOF");
  }

  void test_loadTOF_IN6() {
    checkLoader("ILL/IN6/164192", "LoadILLTOF");
    checkLoader("ILL/IN6/220010", "LoadILLTOF");
  }

  void test_loadTOF_PANTHER() {
    checkLoader("ILL/PANTHER/001036", "LoadILLTOF"); // monochromatic PANTHER
    checkLoader("ILL/PANTHER/001723", "LoadILLTOF");
  }

  void test_loadReflectometry_D17() {
    checkLoader("ILL/D17/317370", "LoadILLReflectometry");
  }

  void test_loadReflectometry_FIGARO() {
    checkLoader("ILL/Figaro/000002", "LoadILLReflectometry");
  }
};

#endif /* MANTID_DATAHANDLING_LOADILLTEST_H_ */
