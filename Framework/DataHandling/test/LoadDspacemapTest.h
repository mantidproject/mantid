// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/LoadDspacemap.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Timer.h"
#include <cstring>
#include <cxxtest/TestSuite.h>
#include <fstream>
#include <vector>

using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using Mantid::DataObjects::OffsetsWorkspace_sptr;
using Mantid::Kernel::ConfigService;

class LoadDspacemapTest : public CxxTest::TestSuite {
public:
  void testINES() {
    LoadDspacemap testerDSP;
    TS_ASSERT_THROWS_NOTHING(testerDSP.initialize());
    TS_ASSERT_THROWS_NOTHING(testerDSP.isInitialized());
    testerDSP.setPropertyValue("InstrumentFilename",
                               ConfigService::Instance().getString("instrumentDefinition.directory") +
                                   "/INES_Definition.xml");
    std::string dspaceFile = "./INES_LoadDspacemaptoCalTest.dat";
    std::ofstream fout("./INES_LoadDspacemaptoCalTest.dat", std::ios_base::out | std::ios_base::binary);
    double read = 3.1992498205034756E-6;
    for (int i = 0; i < 147; i++)
      fout.write(reinterpret_cast<char *>(&read), sizeof read);
    fout.close();
    testerDSP.setPropertyValue("Filename", dspaceFile);
    testerDSP.setPropertyValue("OutputWorkspace", "ines_offsets");
    testerDSP.execute();
    TS_ASSERT(testerDSP.isExecuted());

    // Get the offsets out
    OffsetsWorkspace_sptr offsetsWS =
        std::dynamic_pointer_cast<OffsetsWorkspace>(AnalysisDataService::Instance().retrieve("ines_offsets"));
    TS_ASSERT(offsetsWS);
    if (!offsetsWS)
      return;

    // Check one point.
    TS_ASSERT_DELTA(offsetsWS->dataY(0)[0], -0.6162, 0.0001);
  }

  void doTestVulcan(const std::string &dspaceFile, const std::string &fileType) {
    LoadDspacemap testerDSP;
    TS_ASSERT_THROWS_NOTHING(testerDSP.initialize());
    TS_ASSERT_THROWS_NOTHING(testerDSP.isInitialized());
    testerDSP.setPropertyValue("InstrumentFilename",
                               ConfigService::Instance().getString("instrumentDefinition.directory") +
                                   "/VULCAN_Definition.xml");
    testerDSP.setPropertyValue("Filename", dspaceFile);
    testerDSP.setPropertyValue("FileType", fileType);
    testerDSP.setPropertyValue("OutputWorkspace", "test_vulcan_offset");
    TS_ASSERT_THROWS_NOTHING(testerDSP.execute());
    TS_ASSERT(testerDSP.isExecuted());

    // Get the offsets out
    OffsetsWorkspace_sptr offsetsWS =
        std::dynamic_pointer_cast<OffsetsWorkspace>(AnalysisDataService::Instance().retrieve("test_vulcan_offset"));
    TS_ASSERT(offsetsWS);
    if (!offsetsWS)
      return;

    // Check one point.
    // TS_ASSERT_DELTA( offsetsWS->dataY(0)[0],  0.0938, 0.0001 );
  }

  void testVulcan_ASCII() { doTestVulcan("pid_offset_vulcan_new.dat", "VULCAN-ASCII"); }

  void testVulcan_Binary() { doTestVulcan("pid_offset_vulcan_new.dat.bin", "VULCAN-Binary"); }
};
