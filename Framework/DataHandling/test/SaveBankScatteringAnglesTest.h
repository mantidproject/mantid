// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAVEBANKSCATTERINGANGLESTEST_H_
#define MANTID_DATAHANDLING_SAVEBANKSCATTERINGANGLESTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/SaveBankScatteringAngles.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <Poco/TemporaryFile.h>
#include <cxxtest/TestSuite.h>

#include <fstream>

using namespace Mantid;

using DataHandling::SaveBankScatteringAngles;

class SaveBankScatteringAnglesTest : public CxxTest::TestSuite {

public:
  void test_init() {
    SaveBankScatteringAngles testAlg;
    TS_ASSERT_THROWS_NOTHING(testAlg.initialize());
  }

  void test_inputWorkspaceMustBeGroup() {
    const auto ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    auto &ADS = API::AnalysisDataService::Instance();
    ADS.add("ws", ws);

    SaveBankScatteringAngles testAlg;
    testAlg.initialize();
    TS_ASSERT_THROWS(testAlg.setProperty("InputWorkspace", "ws"),
                     const std::invalid_argument &);

    ADS.remove("ws");
  }

  void test_fileSavedWithCorrectNumberOfLines() {
    auto &ADS = API::AnalysisDataService::Instance();
    const auto ws1 =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 100);
    ADS.add("ws1", ws1);
    const auto ws2 =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 100);
    ADS.add("ws2", ws2);
    groupWorkspaces({"ws1", "ws2"}, "group");

    SaveBankScatteringAngles testAlg;
    testAlg.initialize();
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("InputWorkspace", "group"));

    Poco::TemporaryFile tempFile;
    const std::string &tempFileName = tempFile.path();
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("Filename", tempFileName));
    TS_ASSERT_THROWS_NOTHING(testAlg.execute());

    std::ifstream file(tempFileName);
    std::string line;
    int numLines = 0;

    if (file.is_open()) {
      while (std::getline(file, line)) {
        numLines++;
      }
      TS_ASSERT_EQUALS(numLines, 2);
    } else {
      TS_FAIL("Could not open output file");
    }

    ADS.remove("ws1");
    ADS.remove("ws2");
    ADS.remove("group");
  }

  void test_savedDataIsCorrect() {
    auto load = API::AlgorithmManager::Instance().create("Load");
    load->setProperty("Filename", "ENGINX_277208_focused_bank_2.nxs");
    load->setProperty("OutputWorkspace", "ws");
    load->execute();
    groupWorkspaces({"ws"}, "group");

    SaveBankScatteringAngles testAlg;
    testAlg.initialize();
    testAlg.setProperty("InputWorkspace", "group");

    Poco::TemporaryFile tempFile;
    const std::string &tempFileName = tempFile.path();
    testAlg.setProperty("Filename", tempFileName);
    testAlg.execute();

    std::ifstream file(tempFileName);
    std::string line;

    TS_ASSERT_THROWS_NOTHING(std::getline(file, line));
    TS_ASSERT_EQUALS(
        line,
        "bank :    0  group:     1201    89.9396035211    180.0000000000");

    auto &ADS = API::AnalysisDataService::Instance();
    ADS.remove("ws");
    ADS.remove("group");
  }

private:
  void groupWorkspaces(const std::vector<std::string> &workspaceNames,
                       const std::string &outputWSName) {
    const auto groupAlg =
        API::AlgorithmManager::Instance().create("GroupWorkspaces");
    groupAlg->setProperty("InputWorkspaces", workspaceNames);
    groupAlg->setProperty("OutputWorkspace", outputWSName);
    groupAlg->execute();
  }
};

#endif // MANTID_DATAHANDLING_SAVEBANKSCATTERINGANGLESTEST_H_
