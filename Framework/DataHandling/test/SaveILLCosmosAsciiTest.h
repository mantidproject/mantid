// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAVEILLCOSMOSASCIITEST_H_
#define MANTID_DATAHANDLING_SAVEILLCOSMOSASCIITEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/SaveILLCosmosAscii.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <Poco/File.h>
#include <cxxtest/TestSuite.h>
#include <fstream>
#include <string>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class SaveILLCosmosAsciiTest : public CxxTest::TestSuite {

public:
  static SaveILLCosmosAsciiTest *createSuite() {
    return new SaveILLCosmosAsciiTest();
  }
  static void destroySuite(SaveILLCosmosAsciiTest *suite) { delete suite; }

  SaveILLCosmosAsciiTest() {
    m_filename = "SaveILLCosmosAsciiTestFile.txt";
    m_name = "SaveILLCosmosAsciiWS";
    for (int i = 1; i < m_points + 1; ++i) {
      m_data.push_back(i);
      m_zeros.push_back(0.);
    }
  }
  ~SaveILLCosmosAsciiTest() override {}

  void testInit() {
    SaveILLCosmosAscii alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }

  void testExec() {
    // create a new workspace and then delete it later on
    createWS();

    SaveILLCosmosAscii alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", m_name));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", m_filename));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    m_long_filename = alg.getPropertyValue("Filename");
    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(m_long_filename).exists());
    std::ifstream in(m_long_filename.c_str());
    std::string fullline;
    headingsTests(in, fullline);
    getline(in, fullline);

    std::vector<std::string> columns;
    boost::split(columns, fullline, boost::is_any_of("\t"),
                 boost::token_compress_on);
    TS_ASSERT_EQUALS(columns.size(), 5);
    // the first is black due to the leading separator
    TS_ASSERT_EQUALS(columns[0], "");
    TS_ASSERT_EQUALS(columns[1], "1.500000000000000e+00");
    TS_ASSERT_EQUALS(columns[2], "1.000000000000000e+00");
    TS_ASSERT_EQUALS(columns[3], "1.000000000000000e+00");
    TS_ASSERT_EQUALS(columns[4], "0.000000000000000e+00");
    in.close();

    cleanupafterwards();
  }
  void testNoX() {
    // create a new workspace and then delete it later on
    createWS(true);

    SaveILLCosmosAscii alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", m_name));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", m_filename));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    m_long_filename = alg.getPropertyValue("Filename");
    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(m_long_filename).exists());
    std::ifstream in(m_long_filename.c_str());
    std::string fullline;
    headingsTests(in, fullline);
    getline(in, fullline);
    std::vector<std::string> columns;
    boost::split(columns, fullline, boost::is_any_of("\t"),
                 boost::token_compress_on);
    TS_ASSERT_EQUALS(columns.size(), 5);
    // the first is black due to the leading separator
    TS_ASSERT_EQUALS(columns[0], "");
    TS_ASSERT_EQUALS(columns[1], "0.000000000000000e+00");
    TS_ASSERT_EQUALS(columns[2], "1.000000000000000e+00");
    TS_ASSERT_EQUALS(columns[3], "1.000000000000000e+00");
    TS_ASSERT_EQUALS(columns[4], "0.000000000000000e+00");
    in.close();

    cleanupafterwards();
  }
  void testNoY() {
    // create a new workspace and then delete it later on
    createWS(false, true);

    SaveILLCosmosAscii alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", m_name));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", m_filename));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    m_long_filename = alg.getPropertyValue("Filename");
    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(m_long_filename).exists());
    std::ifstream in(m_long_filename.c_str());
    std::string fullline;
    headingsTests(in, fullline);
    getline(in, fullline);
    std::vector<std::string> columns;
    boost::split(columns, fullline, boost::is_any_of("\t"),
                 boost::token_compress_on);
    TS_ASSERT_EQUALS(columns.size(), 5);
    // the first is black due to the leading separator
    TS_ASSERT_EQUALS(columns[0], "");
    TS_ASSERT_EQUALS(columns[1], "1.500000000000000e+00");
    TS_ASSERT_EQUALS(columns[2], "0.000000000000000e+00");
    TS_ASSERT_EQUALS(columns[3], "1.000000000000000e+00");
    TS_ASSERT_EQUALS(columns[4], "0.000000000000000e+00");
    in.close();

    cleanupafterwards();
  }
  void testNoE() {
    // create a new workspace and then delete it later on
    createWS(false, false, true);

    SaveILLCosmosAscii alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", m_name));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", m_filename));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    m_long_filename = alg.getPropertyValue("Filename");
    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(m_long_filename).exists());
    std::ifstream in(m_long_filename.c_str());
    std::string fullline;
    headingsTests(in, fullline);
    getline(in, fullline);
    std::vector<std::string> columns;
    boost::split(columns, fullline, boost::is_any_of("\t"),
                 boost::token_compress_on);
    TS_ASSERT_EQUALS(columns.size(), 5);
    // the first is black due to the leading separator
    TS_ASSERT(columns.at(0) == "");
    TS_ASSERT_EQUALS(columns[0], "");
    TS_ASSERT_EQUALS(columns[1], "1.500000000000000e+00");
    TS_ASSERT_EQUALS(columns[2], "1.000000000000000e+00");
    TS_ASSERT_EQUALS(columns[3], "0.000000000000000e+00");
    TS_ASSERT_EQUALS(columns[4], "0.000000000000000e+00");
    in.close();

    cleanupafterwards();
  }
  void testParameters() {
    // create a new workspace and then delete it later on
    createWS(false, false, false, true);

    SaveILLCosmosAscii alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", m_name));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", m_filename));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UserContact", "John Smith"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("Title", "Testing this algorithm"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Separator", "comma"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    m_long_filename = alg.getPropertyValue("Filename");
    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(m_long_filename).exists());
    std::ifstream in(m_long_filename.c_str());
    std::string fullline;
    headingsTests(in, fullline, true, ",");
    getline(in, fullline);

    std::vector<std::string> columns;
    boost::split(columns, fullline, boost::is_any_of(","),
                 boost::token_compress_on);
    TS_ASSERT_EQUALS(columns.size(), 5);
    // the first is black due to the leading separator
    TS_ASSERT_EQUALS(columns[0], "");
    TS_ASSERT_EQUALS(columns[1], "1.500000000000000e+00");
    TS_ASSERT_EQUALS(columns[2], "1.000000000000000e+00");
    TS_ASSERT_EQUALS(columns[3], "1.000000000000000e+00");
    TS_ASSERT_EQUALS(columns[4], "0.000000000000000e+00");
    in.close();

    cleanupafterwards();
  }
  void test_fail_invalid_workspace() {
    SaveILLCosmosAscii alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", m_filename));
    m_long_filename = alg.getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_ANYTHING(
        alg.setPropertyValue("InputWorkspace", "NotARealWS"));
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
    TS_ASSERT(!alg.isExecuted());

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(!Poco::File(m_long_filename).exists());
  }

private:
  void headingsTests(std::ifstream &in, std::string &fullline,
                     bool propertiesLogs = false, std::string sep = "\t") {
    getline(in, fullline);
    TS_ASSERT_EQUALS(fullline, "MFT");
    getline(in, fullline);
    TS_ASSERT_EQUALS(fullline, "Instrument: ");
    getline(in, fullline);
    if (propertiesLogs) {
      TS_ASSERT_EQUALS(fullline, "User-local contact: John Smith");
      getline(in, fullline);
      TS_ASSERT_EQUALS(fullline, "Title: Testing this algorithm");
      getline(in, fullline);
      TS_ASSERT_EQUALS(fullline, "Subtitle: ILL COSMOS save test");
      getline(in, fullline);
      TS_ASSERT_EQUALS(fullline, "Start date + time: 2011-12-16T01:27:30");
      getline(in, fullline);
      TS_ASSERT_EQUALS(fullline, "End date + time: 2011-12-16T02:13:31");
    } else {
      TS_ASSERT_EQUALS(fullline, "User-local contact: ");
      getline(in, fullline);
      TS_ASSERT_EQUALS(fullline, "Title: ");
      getline(in, fullline);
      TS_ASSERT_EQUALS(fullline, "Subtitle: ");
      getline(in, fullline);
      TS_ASSERT_EQUALS(fullline, "Start date + time: ");
      getline(in, fullline);
      TS_ASSERT_EQUALS(fullline, "End date + time: ");
    }
    getline(in, fullline);
    TS_ASSERT_EQUALS(fullline, "Number of file format: 2");
    getline(in, fullline);
    std::cout << sep;
    TS_ASSERT_EQUALS(fullline,
                     "Number of data points: " + std::to_string(m_points));
    getline(in, fullline);
    getline(in, fullline);
    TS_ASSERT_EQUALS(fullline, sep + "q" + sep + "refl" + sep + "refl_err" +
                                   sep + "q_res");
  }
  void createWS(bool zeroX = false, bool zeroY = false, bool zeroE = false,
                bool createLogs = false) {
    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::create2DWorkspace(1, m_points);

    if (createLogs) {
      ws->mutableRun().addProperty("run_title",
                                   std::string("ILL COSMOS save test"));
      ws->mutableRun().addProperty("run_start",
                                   std::string("2011-12-16T01:27:30"));
      ws->mutableRun().addProperty("run_end",
                                   std::string("2011-12-16T02:13:31"));
    }

    AnalysisDataService::Instance().addOrReplace(m_name, ws);
    // Check if any of X, Y or E should be zeroed to check for divide by zero or
    // similiar
    if (zeroX)
      std::copy(m_zeros.begin(), m_zeros.end(), ws->mutableX(0).begin());
    else
      std::copy(m_data.begin(), m_data.end(), ws->mutableX(0).begin());

    if (zeroY)
      std::copy(m_zeros.begin(), m_zeros.end(), ws->mutableY(0).begin());
    else
      std::copy(m_data.begin(), m_data.end(), ws->mutableY(0).begin());

    if (zeroE)
      std::copy(m_zeros.begin(), m_zeros.end(), ws->mutableE(0).begin());
    else
      std::copy(m_data.begin(), m_data.end(), ws->mutableE(0).begin());
  }
  void cleanupafterwards() {
    Poco::File(m_long_filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }
  std::string m_filename, m_name, m_long_filename;
  int m_points{2};
  std::vector<double> m_data;
  std::vector<double> m_zeros;
};
#endif /*MANTID_DATAHANDLING_SAVEILLCOSMOSASCIITEST_H_*/
