#ifndef MANTID_DATAHANDLING_SAVEGDATEST_H_
#define MANTID_DATAHANDLING_SAVEGDATEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/SaveGDA.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <Poco/TemporaryFile.h>
#include <cxxtest/TestSuite.h>

#include <fstream>

namespace { // helpers

double computeAverageDeltaTByT(const std::vector<double> &TOF) {
  std::vector<double> deltaTByT;
  deltaTByT.reserve(TOF.size() - 1);
  std::adjacent_difference(TOF.begin(), TOF.end(),
                           std::back_inserter(deltaTByT),
                           [](const double prev, const double curr) {
                             return (prev - curr) / curr;
                           });
  deltaTByT.erase(deltaTByT.begin());
  return std::accumulate(deltaTByT.begin(), deltaTByT.end(), 0.0) /
         static_cast<double>(deltaTByT.size());
}

} // anonymous namespace

using namespace Mantid;

using DataHandling::SaveGDA;

class SaveGDATest : public CxxTest::TestSuite {

public:
  void setUp() override {
    auto load = API::AlgorithmManager::Instance().create("Load");
    load->setProperty("Filename", "ENGINX_277208_focused_bank_2.nxs");
    load->setProperty("OutputWorkspace", TEST_WS_NAME);
    load->execute();

    auto groupAlg = API::AlgorithmManager::Instance().create("GroupWorkspaces");
    groupAlg->setProperty("InputWorkspaces",
                          std::vector<std::string>({TEST_WS_NAME}));
    groupAlg->setProperty("OutputWorkspace", INPUT_GROUP_NAME);
    groupAlg->execute();
  }

  void tearDown() override {
    auto &ADS = API::AnalysisDataService::Instance();
    ADS.remove(TEST_WS_NAME);
    ADS.remove(INPUT_GROUP_NAME);
  }

  void test_init() {
    SaveGDA testAlg;
    TS_ASSERT_THROWS_NOTHING(testAlg.initialize());
  }

  void test_inputWorkspaceMustBeGroup() {
    const auto ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    auto &ADS = API::AnalysisDataService::Instance();
    ADS.add("ws", ws);

    SaveGDA testAlg;
    testAlg.initialize();
    TS_ASSERT_THROWS(testAlg.setProperty("InputWorkspace", "ws"),
                     std::invalid_argument);

    ADS.remove("ws");
  }

  void test_algExecutesWithValidInput() {
    SaveGDA testAlg;

    testAlg.initialize();
    TS_ASSERT_THROWS_NOTHING(
        testAlg.setProperty("InputWorkspace", INPUT_GROUP_NAME));

    Poco::TemporaryFile tempFile;
    const std::string &tempFileName = tempFile.path();
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("Filename", tempFileName));

    TS_ASSERT_THROWS_NOTHING(testAlg.execute());
    TS_ASSERT(testAlg.isExecuted());

    Poco::File shouldExist(tempFileName);
    TS_ASSERT(shouldExist.exists());
  }

  void test_headerValuesAreCorrect() {
    SaveGDA testAlg;
    testAlg.initialize();
    testAlg.setProperty("InputWorkspace", INPUT_GROUP_NAME);
    Poco::TemporaryFile tempFile;
    const std::string &tempFileName = tempFile.path();
    testAlg.setProperty("Filename", tempFileName);
    testAlg.execute();

    std::ifstream file(tempFileName);
    std::string line;
    TS_ASSERT(file.is_open());

    // first line is header
    std::getline(file, line);
    std::vector<std::string> headerItems;
    boost::split(headerItems, line, boost::is_any_of(" "),
                 boost::token_compress_on);

    int numPoints = 0;
    int numLines = 0;
    std::vector<double> TOFs;
    while (std::getline(file, line)) {
      std::vector<std::string> lineItems;
      boost::trim(line);
      boost::split(lineItems, line, boost::is_any_of(" "),
                   boost::token_compress_on);

      // each point has 3 space-separated items on a line
      numPoints += static_cast<int>(lineItems.size()) / 3;
      numLines++;

      for (size_t i = 0; i < lineItems.size(); i += 3) {
        TOFs.emplace_back(std::stod(lineItems[i]));
      }
    }

    const auto expectedNumPoints = std::stoi(headerItems[2]);
    TS_ASSERT_EQUALS(expectedNumPoints, numPoints);

    const auto expectedNumLines = std::stoi(headerItems[3]);
    TS_ASSERT_EQUALS(expectedNumLines, numLines);

    const auto expectedTOFMin1 = std::stoi(headerItems[5]);
    TS_ASSERT_EQUALS(TOFs[0], expectedTOFMin1);

    const auto expectedTOFMin2 = std::stoi(headerItems[7]);
    TS_ASSERT_EQUALS(TOFs[0], expectedTOFMin2);

    const auto averageDeltaTByT = computeAverageDeltaTByT(TOFs);
    const auto expectedAverageDeltaTByT = std::stod(headerItems[8]);
    TS_ASSERT_DELTA(expectedAverageDeltaTByT, averageDeltaTByT, 1e-6);
  }

  void test_dataIsCorrect() {
    SaveGDA testAlg;
    testAlg.initialize();
    testAlg.setProperty("InputWorkspace", INPUT_GROUP_NAME);
    Poco::TemporaryFile tempFile;
    const std::string &tempFileName = tempFile.path();
    testAlg.setProperty("Filename", tempFileName);
    testAlg.execute();

    std::ifstream file(tempFileName);
    std::string line;
    TS_ASSERT(file.is_open());

    // first line is header
    std::getline(file, line);

    std::vector<int> tof;
    std::vector<int> intensity;
    std::vector<int> error;
    while (std::getline(file, line)) {
      std::vector<std::string> lineItems;
      boost::trim(line);
      boost::split(lineItems, line, boost::is_any_of(" "),
                   boost::token_compress_on);
      for (size_t i = 0; i < lineItems.size(); i += 3) {
        TS_ASSERT_THROWS_NOTHING(tof.emplace_back(std::stoi(lineItems[i])));
        TS_ASSERT_THROWS_NOTHING(
            intensity.emplace_back(std::stoi(lineItems[i + 1])));
        TS_ASSERT_THROWS_NOTHING(
            error.emplace_back(std::stoi(lineItems[i + 2])));
      }
    }

    const static size_t expectedNumPoints = 6277;
    TS_ASSERT_EQUALS(tof.size(), expectedNumPoints);
    TS_ASSERT_EQUALS(intensity.size(), expectedNumPoints);
    TS_ASSERT_EQUALS(error.size(), expectedNumPoints);

    // Test a few reference values
    TS_ASSERT_EQUALS(tof[103], 483681);
    TS_ASSERT_EQUALS(intensity[103], 19);
    TS_ASSERT_EQUALS(error[103], 6);

    TS_ASSERT_EQUALS(tof[123], 488784);
    TS_ASSERT_EQUALS(intensity[123], 6);
    TS_ASSERT_EQUALS(error[123], 3);

    TS_ASSERT_EQUALS(tof[3000], 1222874);
    TS_ASSERT_EQUALS(intensity[3000], 71);
    TS_ASSERT_EQUALS(error[3000], 12);
  }

private:
  const static std::string INPUT_GROUP_NAME;
  const static std::string TEST_WS_NAME;

  API::WorkspaceGroup_sptr loadTestWorkspace(const std::string &outputWSName) {
    auto load = API::AlgorithmManager::Instance().create("Load");
    load->setProperty("Filename", "ENGINX_277208_focused_bank_2.nxs");
    load->setProperty("OutputWorkspace", "ws");
    load->execute();
    groupWorkspaces({"ws"}, outputWSName);

    return API::AnalysisDataService::Instance().retrieveWS<API::WorkspaceGroup>(
        outputWSName);
  }

  void groupWorkspaces(const std::vector<std::string> &workspaceNames,
                       const std::string &outputWSName) {
    const auto groupAlg =
        API::AlgorithmManager::Instance().create("GroupWorkspaces");
    groupAlg->setProperty("InputWorkspaces", workspaceNames);
    groupAlg->setProperty("OutputWorkspace", outputWSName);
    groupAlg->execute();
  }
};

const std::string SaveGDATest::TEST_WS_NAME = "SaveGDATestWS";

const std::string SaveGDATest::INPUT_GROUP_NAME = "SaveGDAInputWS";

#endif // MANTID_DATAHANDLING_SAVEGDATEST_H_
