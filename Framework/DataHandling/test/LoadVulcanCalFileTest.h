// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOCALVULCANCALFILETEST_H_
#define MANTID_DATAHANDLING_LOCALVULCANCALFILETEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/LoadVulcanCalFile.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

class LoadVulcanCalFileTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    LoadVulcanCalFile alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void no_test_exec() {
    // Name of the output workspace.
    std::string outWSName("LoadVulcanCalFileTest");
    std::string offsetfilename = "pid_offset_vulcan_new.dat";

    LoadVulcanCalFile alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OffsetFilename", offsetfilename));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Grouping", "6Modules"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("WorkspaceName", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("BankIDs", "21,22,23,26,27,28"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "EffectiveDIFCs", "16372.601900,16376.951300,16372.096300,16336.622200,"
                          "16340.822400,16338.777300"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "Effective2Thetas",
        "90.091000,90.122000,90.089000,89.837000,89.867000,89.852000"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    GroupingWorkspace_sptr groupWS;
    TS_ASSERT_THROWS_NOTHING(
        groupWS = AnalysisDataService::Instance().retrieveWS<GroupingWorkspace>(
            outWSName + "_group"));
    TS_ASSERT(groupWS);
    if (!groupWS)
      return;

    TS_ASSERT_EQUALS(groupWS->getNumberHistograms(), 7392);

    TS_ASSERT_EQUALS(int(groupWS->y(0)[0]), 1);
    TS_ASSERT_EQUALS(int(groupWS->y(7391)[0]), 6);

    // Check if filename is saved
    TS_ASSERT_EQUALS(alg.getPropertyValue("OffsetFilename"),
                     groupWS->run().getProperty("Filename")->value());

    OffsetsWorkspace_sptr offsetsWS;
    TS_ASSERT_THROWS_NOTHING(
        offsetsWS =
            AnalysisDataService::Instance().retrieveWS<OffsetsWorkspace>(
                outWSName + "_offsets"));
    TS_ASSERT(offsetsWS);
    if (!offsetsWS)
      return;

    TS_ASSERT_DELTA(offsetsWS->getValue(26250), -0.000472175, 1e-7);
    TS_ASSERT_DELTA(offsetsWS->y(7391)[0], 6.39813e-05, 1e-7);
    // Check if filename is saved
    TS_ASSERT_EQUALS(alg.getPropertyValue("OffsetFilename"),
                     offsetsWS->run().getProperty("Filename")->value());

    // Masking
    SpecialWorkspace2D_sptr maskWS;
    TS_ASSERT_THROWS_NOTHING(
        maskWS = AnalysisDataService::Instance().retrieveWS<SpecialWorkspace2D>(
            outWSName + "_mask"));
    TS_ASSERT(maskWS);
    if (!maskWS)
      return;

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName + "_group");
    AnalysisDataService::Instance().remove(outWSName + "_offsets");
    AnalysisDataService::Instance().remove(outWSName + "_mask");
    AnalysisDataService::Instance().remove(outWSName + "_TOF_offsets");

    return;
  }

  void no_test_exec2BanksBadPixel() {
    // Name of the output workspace.
    std::string outWSName("LoadVulcanCalFileTest");
    std::string offsetfilename = "pid_offset_vulcan_new.dat";
    std::string badpixelfilename = "bad_pids_vulcan_new_6867_7323.dat";

    LoadVulcanCalFile alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OffsetFilename", offsetfilename));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Grouping", "2Banks"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("BadPixelFilename", badpixelfilename));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("WorkspaceName", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("BankIDs", "21,22,23,26,27,28"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "EffectiveDIFCs", "16376.951300,16376.951300,16376.951300, "
                          "16340.822400,16340.822400,16340.822400"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "Effective2Thetas",
        "90.122000,90.122000,90.122000, 89.867000,89.867000,89.867000"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    GroupingWorkspace_sptr groupWS;
    TS_ASSERT_THROWS_NOTHING(
        groupWS = AnalysisDataService::Instance().retrieveWS<GroupingWorkspace>(
            outWSName + "_group"));
    TS_ASSERT(groupWS);
    if (!groupWS)
      return;

    TS_ASSERT_EQUALS(int(groupWS->getValue(26410)), 1);
    TS_ASSERT_EQUALS(int(groupWS->getValue(34298)), 2);

    // Masking
    SpecialWorkspace2D_sptr maskWS;
    TS_ASSERT_THROWS_NOTHING(
        maskWS = AnalysisDataService::Instance().retrieveWS<SpecialWorkspace2D>(
            outWSName + "_mask"));
    TS_ASSERT(maskWS);
    if (!maskWS)
      return;

    const auto &spectrumInfo = maskWS->spectrumInfo();
    size_t nummasked = 0;
    for (size_t i = 0; i < maskWS->getNumberHistograms(); ++i) {
      if (maskWS->y(i)[0] > 0.5) {
        ++nummasked;
        TS_ASSERT(spectrumInfo.isMasked(i));
      }
    }

    TS_ASSERT_EQUALS(nummasked, 6);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName + "_group");
    AnalysisDataService::Instance().remove(outWSName + "_offsets");
    AnalysisDataService::Instance().remove(outWSName + "_mask");
    AnalysisDataService::Instance().remove(outWSName + "_TOF_offsets");

    return;
  }
};

class LoadVulcanCalFileTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    for (int i = 0; i < numberOfIterations; ++i) {
      loadAlgPtrs.emplace_back(setupAlg());
    }
  }

  void testLoadVulcanCalFilePerformance() {
    for (auto alg : loadAlgPtrs) {
      TS_ASSERT_THROWS_NOTHING(alg->execute());
    }
  }

  void tearDown() override {
    for (int i = 0; i < numberOfIterations; i++) {
      delete loadAlgPtrs[i];
      loadAlgPtrs[i] = nullptr;
    }
    Mantid::API::AnalysisDataService::Instance().remove(outWSName);
  }

private:
  std::vector<LoadVulcanCalFile *> loadAlgPtrs;

  const int numberOfIterations = 1;

  const std::string offsetfilename = "pid_offset_vulcan_new.dat";
  const std::string outWSName = "vulcan_cal_file_ws";
  const std::string effectiveDFCs = "16372.601900,16376.951300,16372.096300,"
                                    "16336.622200, 16340.822400,16338.777300";
  const std::string effective2Thetas =
      "90.091000,90.122000,90.089000,89.837000,89.867000,89.852000";

  LoadVulcanCalFile *setupAlg() {
    LoadVulcanCalFile *loader = new LoadVulcanCalFile;
    loader->initialize();
    loader->isInitialized();
    loader->setPropertyValue("OffsetFilename", offsetfilename);
    loader->setPropertyValue("WorkspaceName", outWSName);
    loader->setPropertyValue("OffsetFilename", offsetfilename);
    loader->setPropertyValue("Grouping", "6Modules");
    loader->setPropertyValue("WorkspaceName", outWSName);
    loader->setPropertyValue("BankIDs", "21,22,23,26,27,28");
    loader->setPropertyValue("EffectiveDIFCs", effectiveDFCs);
    loader->setPropertyValue("Effective2Thetas", effective2Thetas);
    loader->setRethrows(true);

    return loader;
  }
};
#endif /* MANTID_DATAHANDLING_LOCALVULCANCALFILETEST_H_ */
