// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataHandling/GroupDetectors.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidMuon/AlphaCalc.h"
#include "MantidMuon/LoadMuonNexus2.h"
#include <stdexcept>

using namespace Mantid::Algorithms;
using namespace Mantid::API;

class AlphaCalcTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(alphaCalc.name(), "AlphaCalc") }

  void testCategory() { TS_ASSERT_EQUALS(alphaCalc.category(), "Muon") }

  void testInit() {
    alphaCalc.initialize();
    TS_ASSERT(alphaCalc.isInitialized())
  }

  void testCalAlphaManySpectra() {
    // system("pause");
    // Load the muon nexus file
    loader.initialize();
    loader.setPropertyValue("Filename", "emu00006473.nxs");
    loader.setPropertyValue("OutputWorkspace", "EMU6473");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT_EQUALS(loader.isExecuted(), true);

    alphaCalc.setPropertyValue("InputWorkspace", "EMU6473");
    alphaCalc.setPropertyValue("ForwardSpectra", "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16");
    alphaCalc.setPropertyValue("BackwardSpectra", "17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32");
    alphaCalc.setPropertyValue("FirstGoodValue", "0.3");

    try {
      TS_ASSERT_EQUALS(alphaCalc.execute(), true);
    } catch (std::runtime_error &e) {
      TS_FAIL(e.what());
    }
    double alpha = alphaCalc.getProperty("Alpha");
    TS_ASSERT_DELTA(alpha, 1.7875, 0.0001);
  }

  void testCalAlphaTwoSpectra() {
    // Load the muon nexus file
    loader.initialize();
    loader.setPropertyValue("Filename", "emu00006473.nxs");
    loader.setPropertyValue("OutputWorkspace", "EMU6473");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT_EQUALS(loader.isExecuted(), true);

    alphaCalc.setPropertyValue("InputWorkspace", "EMU6473");
    alphaCalc.setPropertyValue("ForwardSpectra", "1");
    alphaCalc.setPropertyValue("BackwardSpectra", "17");
    alphaCalc.setPropertyValue("FirstGoodValue", "0.3");

    try {
      TS_ASSERT_EQUALS(alphaCalc.execute(), true);
    } catch (std::runtime_error &e) {
      TS_FAIL(e.what());
    }
    double alpha = alphaCalc.getProperty("Alpha");
    TS_ASSERT_DELTA(alpha, 1.6880, 0.0001);
  }

  // These tests need extended names once we decide how the given situation
  // should be handled.
  // The tests also need completing

  void test_first_good_value_out_of_range() {}

  void test_last_good_value_out_of_range() {}

  void test_empty_workspace() {}

  void test_workspace_with_all_zeros() {}

  void test_incorrect_spectra_numbers() {}

private:
  AlphaCalc alphaCalc;
  Mantid::Algorithms::LoadMuonNexus2 loader;
};
