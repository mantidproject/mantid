// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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

  void setUp() override { ConfigService::Instance().setString("default.facility", "ILL"); }

  void tearDown() override {

    ConfigService::Instance().setString("default.facility", " ");

    AnalysisDataService::Instance().clear();
  }

  void checkLoader(const std::string &filename, std::string resultLoader) {
    Load alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename))
    TS_ASSERT_EQUALS(alg.getPropertyValue("LoaderName"), resultLoader)
  }

  void test_LoadSANS_D11() {
    std::cout << "Testing LoadSANS for D11" << std::endl;
    checkLoader("ILL/D11/010560", "LoadILLSANS");
  }

  void test_LoadSANS_D33() {
    std::cout << "Testing LoadSANS for D33" << std::endl;
    checkLoader("ILL/D33/002294", "LoadILLSANS");
    checkLoader("ILL/D33/042610", "LoadILLSANS"); // D33 TOF
  }

  void test_LoadSANS_D22() {
    std::cout << "Testing LoadSANS for D22" << std::endl;
    checkLoader("ILL/D22/192068", "LoadILLSANS");
  }

  void test_LoadSANS_D16() {
    std::cout << "Testing LoadSANS for D16" << std::endl;
    checkLoader("ILL/D16/023583", "LoadILLSANS");
    checkLoader("ILL/D16/218356", "LoadILLSANS");
  }

  void test_LoadSANS_D16B() {
    std::cout << "Testing LoadSANS for D16B" << std::endl;
    checkLoader("ILL/D16/066321", "LoadILLSANS");
  }

  void test_LoadDiffraction_D1B() {
    std::cout << "Testing LoadDiffraction for D1B" << std::endl;
    checkLoader("ILL/D1B/473432", "LoadILLDiffraction");
  }

  void test_LoadDiffraction_D2B() {
    std::cout << "Testing LoadDiffraction for D2B" << std::endl;
    checkLoader("ILL/D2B/535401", "LoadILLDiffraction");
  }

  void test_LoadDiffraction_D4() {
    std::cout << "Testing LoadDiffraction for D4" << std::endl;
    checkLoader("ILL/D4/387230", "LoadILLDiffraction");
  }

  void test_LoadDiffraction_D20() {
    std::cout << "Testing LoadDiffraction for D20" << std::endl;
    checkLoader("ILL/D20/967076", "LoadILLDiffraction");
    checkLoader("ILL/D20/967087", "LoadILLDiffraction");
  }

  void test_LoadPolarizedDiffraction_D7() {
    std::cout << "Testing LoadPolarizedDiffraction for D7" << std::endl;
    checkLoader("ILL/D7/394458", "LoadILLPolarizedDiffraction");
  }

  void test_loadIndirect_IN16B() {
    std::cout << "Testing LoadIndirect for IN16B" << std::endl;
    std::cout << "load one wing qens" << std::endl;
    checkLoader("ILL/IN16B/090661", "LoadILLIndirect"); // one wing qens
    std::cout << "load one wing efws" << std::endl;
    checkLoader("ILL/IN16B/083072", "LoadILLIndirect"); // one wing efws
    std::cout << "load one wing ifws" << std::endl;
    checkLoader("ILL/IN16B/083073", "LoadILLIndirect"); // one wing ifws
    std::cout << "load two wing qens" << std::endl;
    checkLoader("ILL/IN16B/136558", "LoadILLIndirect"); // two wings qens
    std::cout << "load two wing efws" << std::endl;
    checkLoader("ILL/IN16B/143720", "LoadILLIndirect"); // two wings efws
    std::cout << "load two wing ifws" << std::endl;
    checkLoader("ILL/IN16B/170300", "LoadILLIndirect"); // two wings ifws
    std::cout << "load bats" << std::endl;
    checkLoader("ILL/IN16B/215962", "LoadILLIndirect"); // bats
  }

  void test_loadTOF_IN4() {
    std::cout << "Testing LoadTOF for IN4" << std::endl;
    checkLoader("ILL/IN4/084446", "LoadILLTOF");
  }

  void test_loadTOF_IN5() {
    std::cout << "Testing LoadTOF for IN5" << std::endl;
    checkLoader("ILL/IN5/104007", "LoadILLTOF");
    checkLoader("ILL/IN5/189171", "LoadILLTOF");
    checkLoader("ILL/IN5/199857", "LoadILLTOF"); // scan IN5
  }

  void test_loadTOF_IN6() {
    std::cout << "Testing LoadTOF for IN6" << std::endl;
    checkLoader("ILL/IN6/164192", "LoadILLTOF");
    checkLoader("ILL/IN6/220010", "LoadILLTOF");
  }

  void test_loadTOF_PANTHER() {
    std::cout << "Testing LoadTOF for PANTHER" << std::endl;
    checkLoader("ILL/PANTHER/001036", "LoadILLTOF"); // monochromatic PANTHER
    checkLoader("ILL/PANTHER/001723", "LoadILLTOF");
    checkLoader("ILL/PANTHER/010578", "LoadILLTOF"); // scan PANTHER
  }

  void test_loadTOF_SHARP() {
    std::cout << "Testing LoadTOF for SHARP" << std::endl;
    checkLoader("ILL/SHARP/000102", "LoadILLTOF"); // single-channel
    checkLoader("ILL/SHARP/000103", "LoadILLTOF");
    checkLoader("ILL/SHARP/000104.nxs", "LoadILLTOF"); // scan SHARP
  }

  void test_loadReflectometry_D17() {
    std::cout << "Testing LoadReflectometry for D17" << std::endl;
    checkLoader("ILL/D17/317370", "LoadILLReflectometry");
  }

  void test_loadReflectometry_FIGARO() {
    std::cout << "Testing LoadReflectometry for FIGARO" << std::endl;
    checkLoader("ILL/Figaro/000002", "LoadILLReflectometry");
  }

  void test_loadSALSA() {
    std::cout << "Testing LoadSALSA" << std::endl;
    checkLoader("ILL/SALSA/046430", "LoadILLSALSA");
    checkLoader("ILL/SALSA/046508", "LoadILLSALSA");
  }

  void test_loadLagrange() {
    std::cout << "Testing LoadILLLagrange" << std::endl;
    checkLoader("ILL/LAGRANGE/014412", "LoadILLLagrange");
  }
};
