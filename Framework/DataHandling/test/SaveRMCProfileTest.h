// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <filesystem>
#include <fstream>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataHandling/SaveRMCProfile.h"

using Mantid::DataHandling::LoadNexusProcessed;
using Mantid::DataHandling::SaveRMCProfile;
using namespace Mantid::API;

class SaveRMCProfileTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveRMCProfileTest *createSuite() { return new SaveRMCProfileTest(); }
  static void destroySuite(SaveRMCProfileTest *suite) { delete suite; }

  void test_Init() {
    SaveRMCProfile alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  size_t read(std::istream &is, std::vector<char> &buff) {
    is.read(&buff[0], buff.size());
    return is.gcount();
  }

  size_t countEOL(const std::vector<char> &buff, size_t sz) {
    size_t newlines = 0;
    const char *p = &buff[0];
    for (size_t i = 0; i < sz; i++) {
      if (p[i] == '\n') {
        newlines++;
      }
    }
    return newlines;
  }

  size_t countLines(const std::filesystem::path &filepath) {
    const size_t BUFFER_SIZE = 1024 * 1024;
    std::vector<char> buffer(BUFFER_SIZE);
    std::ifstream in(filepath);
    size_t n = 0;
    while (size_t cc = read(in, buffer)) {
      n += countEOL(buffer, cc);
    }
    return n;
  }

  bool loadWorkspace(const std::string &filename, const std::string &wsName) {
    LoadNexusProcessed load;
    load.initialize();
    load.setProperty("Filename", filename);
    load.setProperty("OutputWorkspace", wsName);
    load.execute();
    return load.isExecuted();
  }

  void test_exec() {
    // name of workspace to create and save
    const std::string wsName("SaveRMCProfileTest_OutputWS");
    // name of the output file
    const std::string outFilename("SaveRMCProfileTest_Output.fq");

    // Load a file to save out
    TS_ASSERT(loadWorkspace("nom_gr.nxs", wsName));

    // save the file
    SaveRMCProfile alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputType", "S(Q)"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Title", "nom_gr"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", outFilename));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // do the checks
    std::filesystem::path outFile(outFilename);
    TS_ASSERT(std::filesystem::is_regular_file(outFile));
    TS_ASSERT_EQUALS(countLines(outFile), 1002);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(wsName);

    // remove the output file
    std::filesystem::remove(outFile);
  }

  void test_exec_ws_group() {
    // Create a group
    const std::string groupName("SaveRMCProfileGroup");
    TS_ASSERT(loadWorkspace("nom_gr.nxs", groupName + "_1"));
    TS_ASSERT(loadWorkspace("nom_gr.nxs", groupName + "_2"));

    auto grpAlg = AlgorithmManager::Instance().createUnmanaged("GroupWorkspaces");
    grpAlg->initialize();
    grpAlg->setPropertyValue("InputWorkspaces", groupName + "_1," + groupName + "_2");
    grpAlg->setPropertyValue("OutputWorkspace", groupName);
    grpAlg->execute();

    // name of the output file
    const std::string outFilename("SaveRMCProfileGroup.gr");

    // run the algorithm with a group
    SaveRMCProfile alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", groupName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", outFilename));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // do the checks
    std::filesystem::path outFile = std::filesystem::current_path() / outFilename;
    std::string const outFileString = outFile.string();
    TSM_ASSERT(outFileString + " does not exist", std::filesystem::exists(outFile));
    TSM_ASSERT(outFileString + " is not a regular file", std::filesystem::is_regular_file(outFile));
    TS_ASSERT_EQUALS(countLines(outFile), 1002);

    // remove the workspace group
    AnalysisDataService::Instance().deepRemoveGroup(groupName);
    // remove the output file
    std::filesystem::remove(outFile);
  }
};
