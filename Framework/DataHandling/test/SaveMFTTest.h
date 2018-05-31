#ifndef MANTID_DATAHANDLING_SAVEMFTTEST_H_
#define MANTID_DATAHANDLING_SAVEMFTTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/SaveMFT.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <Poco/File.h>
#include <string>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class SaveMFTTest : public CxxTest::TestSuite {

public:
  static SaveMFTTest *createSuite() { return new SaveMFTTest(); }
  static void destroySuite(SaveMFTTest *suite) { delete suite; }

  SaveMFTTest() {
    m_filename = "SaveMFTTestFile.txt";
    m_name = "SaveMFTWS";
    for (int i = 1; i < m_points + 1; ++i) {
      m_data.push_back(i);
      m_zeros.push_back(0);
    }
  }
  ~SaveMFTTest() override {}

  void testInit() {
    SaveMFT alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }

  void test_invalid_InputWorkspace() {
    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", "ws"));
    TS_ASSERT_THROWS(alg.setProperty("InputWorkspace", "abc"),
                     std::invalid_argument);
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_point_data() {
    const auto &x1 = Mantid::HistogramData::Points({0.33, 0.34});
    const auto &y1 = Mantid::HistogramData::Counts({3., 6.6});
    Mantid::HistogramData::Histogram histogram(x1, y1);
    auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();
    ws->initialize(1, histogram);
    std::string wsname = "ws1";
    AnalysisDataService::Instance().addOrReplace(wsname, ws);
    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", wsname));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", wsname));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename).exists());
    std::vector<std::string> data;
    data.reserve(5);
    data.emplace_back("MFT");
    data.emplace_back("q\trefl\trefl_err\tq_res");
    data.emplace_back("3.300000e-01\t3.000000e+00\t1.732051e+00\t0.000000e+00");
    data.emplace_back("3.400000e-01\t6.600000e+00\t2.569047e+00\t0.000000e+00");
    std::ifstream in(filename);
    std::string fullline;
    auto it = data.begin();
    while (std::getline(in, fullline)) {
      if (in.ignore(std::numeric_limits<std::streamsize>::max(), '\t')) {
        TS_ASSERT_EQUALS(fullline, *it);
        ++it;
      }
    }
    TS_ASSERT(in.eof());
    // TS_ASSERT_EQUALS(it, data.end());
    try {
      Poco::File(filename).remove();
    } catch (...) {
      TS_FAIL("Error deleting file " + filename);
    }
    AnalysisDataService::Instance().remove(wsname);
  }

  void test_histogram_data() {
    const auto &x1 = Mantid::HistogramData::BinEdges({2.4, 3.7, 10.8});
    const auto &y1 = Mantid::HistogramData::Counts({3., 6.6});
    Mantid::HistogramData::Histogram histogram(x1, y1);
    auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();
    ws->initialize(1, histogram);
    std::string wsname = "ws1";
    AnalysisDataService::Instance().addOrReplace(wsname, ws);
    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", wsname));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", wsname));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename).exists());
    std::vector<std::string> data;
    data.reserve(5);
    data.emplace_back("MFT");
    data.emplace_back("q\trefl\trefl_err\tq_res");
    data.emplace_back("3.050000e+00\t3.000000e+00\t1.732051e+00\t0.000000e+00");
    data.emplace_back("7.250000e+00\t6.600000e+00\t2.569047e+00\t0.000000e+00");
    std::ifstream in(filename);
    std::string fullline;
    auto it = data.begin();
    while (std::getline(in, fullline)) {
      if (in.ignore(std::numeric_limits<std::streamsize>::max(), '\t')) {
        TS_ASSERT_EQUALS(fullline, *it);
        ++it;
      }
    }
    TS_ASSERT(in.eof());
    try {
      Poco::File(filename).remove();
    } catch (...) {
      TS_FAIL("Error deleting file " + filename);
    }
    AnalysisDataService::Instance().remove(wsname);
  }

  void test_number_of_header_lines_no_data() {
    auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();
    std::string wsname = "ws1";
    AnalysisDataService::Instance().addOrReplace(wsname, ws);
    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", wsname));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", wsname));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename).exists());
    std::ifstream in(filename);
    // Header: number of lines
    TS_ASSERT_EQUALS(std::count(std::istreambuf_iterator<char>(in),
                                std::istreambuf_iterator<char>(), '\n'),
                     13);
    try {
      Poco::File(filename).remove();
    } catch (...) {
      TS_FAIL("Error deleting file " + filename);
    }
    AnalysisDataService::Instance().remove(wsname);
  }

  void test_number_lines_for_two_data_values() {
    const auto &x1 = Mantid::HistogramData::Points({0.33, 0.34});
    const auto &y1 = Mantid::HistogramData::Counts({3., 6.6});
    Mantid::HistogramData::Histogram histogram(x1, y1);
    auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();
    ws->initialize(1, histogram);
    std::string wsname = "ws1";
    AnalysisDataService::Instance().addOrReplace(wsname, ws);
    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", wsname));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", wsname));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename).exists());
    std::ifstream in(filename);
    // Total number of lines
    TS_ASSERT_EQUALS(std::count(std::istreambuf_iterator<char>(in),
                                std::istreambuf_iterator<char>(),
                                in.widen('\n')),
                     13);
    try {
      Poco::File(filename).remove();
    } catch (...) {
      TS_FAIL("Error deleting file " + filename);
    }
    AnalysisDataService::Instance().remove(wsname);
  }

  void testExec() {
    // create a new workspace and then delete it later on
    createWS();

    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
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
    TS_ASSERT_EQUALS(columns[1], "1.500000e+00");
    TS_ASSERT_EQUALS(columns[2], "1.000000e+00");
    TS_ASSERT_EQUALS(columns[3], "1.000000e+00");
    TS_ASSERT_EQUALS(columns[4], "0.000000e+00");
    try {
      Poco::File(m_long_filename).remove();
    } catch (...) {
    }
    AnalysisDataService::Instance().remove(m_name);
  }

  void testNoX() {
    // create a new workspace and then delete it later on
    createWS(true);

    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
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
    TS_ASSERT_EQUALS(columns[1], "0.000000e+00");
    TS_ASSERT_EQUALS(columns[2], "1.000000e+00");
    TS_ASSERT_EQUALS(columns[3], "1.000000e+00");
    TS_ASSERT_EQUALS(columns[4], "0.000000e+00");
    try {
      Poco::File(m_long_filename).remove();
    } catch (...) {
    }
    AnalysisDataService::Instance().remove(m_name);
  }
  void testNoY() {
    // create a new workspace and then delete it later on
    createWS(false, true);

    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
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
    TS_ASSERT_EQUALS(columns[1], "1.500000e+00");
    TS_ASSERT_EQUALS(columns[2], "0.000000e+00");
    TS_ASSERT_EQUALS(columns[3], "1.000000e+00");
    TS_ASSERT_EQUALS(columns[4], "0.000000e+00");
    try {
      Poco::File(m_long_filename).remove();
    } catch (...) {
    }
    AnalysisDataService::Instance().remove(m_name);
  }
  void testNoE() {
    // create a new workspace and then delete it later on
    createWS(false, false, true);

    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
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
    TS_ASSERT_EQUALS(columns[1], "1.500000e+00");
    TS_ASSERT_EQUALS(columns[2], "1.000000e+00");
    TS_ASSERT_EQUALS(columns[3], "0.000000e+00");
    TS_ASSERT_EQUALS(columns[4], "0.000000e+00");
    try {
      Poco::File(m_long_filename).remove();
    } catch (...) {
    }
    AnalysisDataService::Instance().remove(m_name);
  }
  void testParameters() {
    // create a new workspace and then delete it later on
    createWS(false, false, false, true);

    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
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
    TS_ASSERT_EQUALS(columns[1], "1.500000e+00");
    TS_ASSERT_EQUALS(columns[2], "1.000000e+00");
    TS_ASSERT_EQUALS(columns[3], "1.000000e+00");
    TS_ASSERT_EQUALS(columns[4], "0.000000e+00");
    try {
      Poco::File(m_long_filename).remove();
    } catch (...) {
    }
    AnalysisDataService::Instance().remove(m_name);
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
  std::string m_filename, m_name, m_long_filename;
  int m_points{2};
  std::vector<double> m_data;
  std::vector<double> m_zeros;
};
#endif /*MANTID_DATAHANDLING_SAVEMFTTEST_H_*/
