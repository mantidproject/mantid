// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/SaveGDA.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include <Poco/TemporaryFile.h>
#include <boost/algorithm/string/predicate.hpp>
#include <cxxtest/TestSuite.h>

#include <fstream>

namespace { // helpers

double computeAverageDeltaTByT(const std::vector<double> &TOF) {
  std::vector<double> deltaTByT;
  deltaTByT.reserve(TOF.size() - 1);
  std::adjacent_difference(TOF.begin(), TOF.end(), std::back_inserter(deltaTByT),
                           [](const double prev, const double curr) { return (prev - curr) / curr; });
  deltaTByT.erase(deltaTByT.begin());
  return std::accumulate(deltaTByT.begin(), deltaTByT.end(), 0.0) / static_cast<double>(deltaTByT.size());
}

} // anonymous namespace

using namespace Mantid;

using DataHandling::SaveGDA;

class SaveGDATest : public CxxTest::TestSuite {

public:
  static SaveGDATest *createSuite() { return new SaveGDATest(); }

  static void destroySuite(SaveGDATest *suite) { delete suite; }

  SaveGDATest() {
    const auto &paramsFilePath = m_paramsFile.path();
    std::ofstream paramsFile(paramsFilePath);
    if (!paramsFile) {
      throw std::runtime_error("Could not create GSAS params file: " + paramsFilePath);
    }
    paramsFile << PARAMS_FILE_TEXT;

    const static std::string spectrum1Name = "spectrum1";
    createSampleWorkspace("name=Gaussian,Height=1,PeakCentre=10,Sigma=1;name="
                          "Gaussian,Height=0.8,PeakCentre=5,Sigma=0.8",
                          spectrum1Name);

    const static std::string spectrum2Name = "spectrum2";
    createSampleWorkspace("name=Gaussian,Height=0.8,PeakCentre=5,Sigma=0.8;"
                          "name=Gaussian,Height=1,PeakCentre=10,Sigma=1",
                          spectrum2Name);

    groupWorkspaces({spectrum1Name, spectrum2Name}, INPUT_GROUP_NAME);
  }

  ~SaveGDATest() {
    auto &ADS = API::AnalysisDataService::Instance();
    ADS.remove(INPUT_GROUP_NAME);
    ADS.remove(SPECTRUM_1_NAME);
    ADS.remove(SPECTRUM_2_NAME);
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
    TS_ASSERT_THROWS(testAlg.setProperty("InputWorkspace", "ws"), const std::invalid_argument &);

    ADS.remove("ws");
  }

  void test_groupingSchemeMustMatchNumberOfSpectra() {
    SaveGDA testAlg;
    testAlg.initialize();
    testAlg.setProperty("InputWorkspace", INPUT_GROUP_NAME);
    // This should make the algorithm throw, as there are 2 spectra but three
    // values in the grouping scheme
    testAlg.setProperty("GroupingScheme", std::vector<int>{1, 2, 3});
    testAlg.setProperty("GSASParamFile", m_paramsFile.path());

    Poco::TemporaryFile tempFile;
    const std::string &tempFileName = tempFile.path();
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("OutputFilename", tempFileName));

    TS_ASSERT_THROWS_ANYTHING(testAlg.execute());
  }

  void test_algExecutesWithValidInput() {
    SaveGDA testAlg;

    testAlg.initialize();
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("InputWorkspace", INPUT_GROUP_NAME));
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("GSASParamFile", m_paramsFile.path()));
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("GroupingScheme", std::vector<int>{1, 2}));

    Poco::TemporaryFile tempFile;
    const std::string &tempFileName = tempFile.path();
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("OutputFilename", tempFileName));

    TS_ASSERT_THROWS_NOTHING(testAlg.execute());
    TS_ASSERT(testAlg.isExecuted());

    Poco::File shouldExist(tempFileName);
    TS_ASSERT(shouldExist.exists());
  }

  void test_headerValuesAreCorrect() {
    SaveGDA testAlg;
    testAlg.initialize();
    testAlg.setProperty("InputWorkspace", INPUT_GROUP_NAME);
    testAlg.setProperty("GSASParamFile", m_paramsFile.path());
    testAlg.setProperty("GroupingScheme", std::vector<int>({1, 2}));
    Poco::TemporaryFile tempFile;
    const std::string &tempFileName = tempFile.path();
    testAlg.setProperty("OutputFilename", tempFileName);
    testAlg.execute();

    std::ifstream file(tempFileName);
    std::string line;
    TS_ASSERT(file.is_open());

    // first line is header
    std::getline(file, line);
    TS_ASSERT(line.starts_with("BANK 1"));
    std::vector<std::string> headerItems;
    boost::split(headerItems, line, boost::is_any_of(" "), boost::token_compress_on);

    int numPoints = 0;
    int numLines = 0;
    std::vector<double> TOFs;
    while (std::getline(file, line) && !line.starts_with("BANK")) {
      std::vector<std::string> lineItems;
      boost::trim(line);
      boost::split(lineItems, line, boost::is_any_of(" "), boost::token_compress_on);

      // each point has 3 space-separated items on a line
      numPoints += static_cast<int>(lineItems.size()) / 3;
      numLines++;

      for (size_t i = 0; i < lineItems.size(); i += 3) {
        TOFs.emplace_back(std::stod(lineItems[i]));
      }
    }

    TS_ASSERT_EQUALS(headerItems.size(), 11);
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
    TS_ASSERT_DELTA(expectedAverageDeltaTByT, averageDeltaTByT, 1e-3);

    // Just make sure there's another header after the one we just checked
    TS_ASSERT(line.starts_with("BANK 2"));
  }

  void test_dataIsCorrect() {
    SaveGDA testAlg;
    testAlg.initialize();
    testAlg.setProperty("InputWorkspace", INPUT_GROUP_NAME);
    testAlg.setProperty("GSASParamFile", m_paramsFile.path());
    testAlg.setProperty("GroupingScheme", std::vector<int>({1, 2}));
    Poco::TemporaryFile tempFile;
    const std::string &tempFileName = tempFile.path();
    testAlg.setProperty("OutputFilename", tempFileName);
    testAlg.execute();

    std::ifstream file(tempFileName);
    std::string line;
    TS_ASSERT(file.is_open());

    // first line is header
    std::getline(file, line);

    std::vector<int> tof;
    std::vector<int> intensity;
    std::vector<int> error;
    while (std::getline(file, line) && !line.starts_with("BANK")) {
      std::vector<std::string> lineItems;
      boost::trim(line);
      boost::split(lineItems, line, boost::is_any_of(" "), boost::token_compress_on);
      for (size_t i = 0; i < lineItems.size(); i += 3) {
        TS_ASSERT_THROWS_NOTHING(tof.emplace_back(std::stoi(lineItems[i])));
        TS_ASSERT_THROWS_NOTHING(intensity.emplace_back(std::stoi(lineItems[i + 1])));
        TS_ASSERT_THROWS_NOTHING(error.emplace_back(std::stoi(lineItems[i + 2])));
      }
    }

    const static size_t expectedNumPoints = 13000;
    TS_ASSERT_EQUALS(tof.size(), expectedNumPoints);
    TS_ASSERT_EQUALS(intensity.size(), expectedNumPoints);
    TS_ASSERT_EQUALS(error.size(), expectedNumPoints);

    // Test a few reference values
    TS_ASSERT_EQUALS(tof[103], 49920);
    TS_ASSERT_EQUALS(intensity[103], 1);
    TS_ASSERT_EQUALS(error[103], 34);

    TS_ASSERT_EQUALS(tof[123], 50398);
    TS_ASSERT_EQUALS(intensity[123], 1);
    TS_ASSERT_EQUALS(error[123], 35);

    TS_ASSERT_EQUALS(tof[3000], 119009);
    TS_ASSERT_EQUALS(intensity[3000], 800);
    TS_ASSERT_EQUALS(error[3000], 894);
  }

private:
  const static std::string SPECTRUM_1_NAME;
  const static std::string SPECTRUM_2_NAME;
  const static std::string INPUT_GROUP_NAME;
  const static std::string PARAMS_FILE_TEXT;

  Poco::TemporaryFile m_paramsFile;

  void createSampleWorkspace(const std::string &function, const std::string &outputWSName) const {
    auto &algorithmManager = API::AlgorithmManager::Instance();
    const auto createAlg = algorithmManager.create("CreateSampleWorkspace");
    createAlg->setProperty("Function", "User Defined");
    createAlg->setProperty("UserDefinedFunction", function);
    createAlg->setProperty("NumBanks", "1");
    createAlg->setProperty("XUnit", "dSpacing");
    createAlg->setProperty("XMin", "2");
    createAlg->setProperty("XMax", "15");
    createAlg->setProperty("BinWidth", "0.001");
    createAlg->setProperty("OutputWorkspace", outputWSName);
    createAlg->execute();

    const auto extractAlg = algorithmManager.create("ExtractSingleSpectrum");
    extractAlg->setProperty("InputWorkspace", outputWSName);
    extractAlg->setProperty("OutputWorkspace", outputWSName);
    extractAlg->setProperty("WorkspaceIndex", "0");
    extractAlg->execute();
  }

  void groupWorkspaces(const std::vector<std::string> &workspaceNames, const std::string &outputWSName) const {
    const auto groupAlg = API::AlgorithmManager::Instance().create("GroupWorkspaces");
    groupAlg->setProperty("InputWorkspaces", workspaceNames);
    groupAlg->setProperty("OutputWorkspace", outputWSName);
    groupAlg->execute();
  }
};

const std::string SaveGDATest::INPUT_GROUP_NAME = "SaveGDAInputWS";

const std::string SaveGDATest::SPECTRUM_1_NAME = "Spectrum1";

const std::string SaveGDATest::SPECTRUM_2_NAME = "Spectrum2";

const std::string SaveGDATest::PARAMS_FILE_TEXT =
    "COMM  GEM84145\n"
    "INS   BANK\n"
    "INS   HTYPE   PNTR\n"
    "INS  1 ICONS    746.96     -0.24     -9.78\n"
    "INS  1BNKPAR    2.3696      9.39      0.00    .00000     .3000    1    1\n"
    "INS  1I ITYP    0    1.000     25.000\n"
    "INS  1PRCF      1   12   0.00100\n"
    "INS  1PRCF 1   0.000000E+00   0.163590E+00   0.265000E-01   0.210800E-01\n"
    "INS  1PRCF 2   0.000000E+00   0.900816E+02   0.000000E+00   0.000000E+00\n"
    "INS  1PRCF 3   0.000000E+00   0.000000E+00   0.000000E+00   0.000000E+00\n"
    "INS  2 ICONS   1468.19      4.82      8.95   AZ\n"
    "INS  2BNKPAR    1.7714     17.98      0.00    .00000     .3000    1    1\n"
    "INS  2I ITYP    0    1.000     21.000       2\n"
    "INS  2PRCF      1   12   0.00100\n"
    "INS  2PRCF 1   0.000000E+00   0.163590E+00   0.265000E-01   0.210800E-01\n"
    "INS  2PRCF 2   0.000000E+00   0.151242E+03   0.103200E+02   0.000000E+00\n"
    "INS  2PRCF 3   0.000000E+00   0.000000E+00   0.000000E+00\n"
    "0.000000E+00\n";
