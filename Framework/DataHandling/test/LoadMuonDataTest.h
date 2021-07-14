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
