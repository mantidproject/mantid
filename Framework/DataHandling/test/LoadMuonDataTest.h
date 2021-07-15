// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadMuonData.h"

using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;

class LoadMuonDataTest : public CxxTest::TestSuite {

public:
  void testExecWithNexusFile() {
    loader.initialize();
    loader.setProperty("Filename", "emu00006473.nxs");
    loader.setProperty("OutputWorkspace", "OutWS");

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());
    std::string field = loader.getProperty("MainFieldDirection");
    TS_ASSERT_EQUALS("Longitudinal", field);
    double timeZero = loader.getProperty("TimeZero");
    TS_ASSERT_DELTA(timeZero, 0.55, 0.001);
    double firstgood = loader.getProperty("FirstGoodData");
    TS_ASSERT_DELTA(firstgood, 0.656, 0.001);
  }

  void testExecWithBinFile() {
    loader.initialize();
    loader.setProperty("Filename", "deltat_tdc_dolly_1529.bin");
    loader.setProperty("OutputWorkspace", "OutWS");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
  }

private:
  LoadMuonData loader;
};
