#ifndef MANTID_DATAHANDLING_SAVEMFTTEST_H_
#define MANTID_DATAHANDLING_SAVEMFTTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/SaveMFT.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
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
    const Workspace_sptr ws = create<Workspace2D>(1, histogram);
    const auto ws2{boost::dynamic_pointer_cast<Workspace>(ws)};
    const std::string wsname = "ws1";
    AnalysisDataService::Instance().addOrReplace(wsname, ws2);
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
    Workspace2D_sptr ws = create<Workspace2D>(1, histogram);
    const std::string wsname = "ws1";
    const auto ws2{boost::dynamic_pointer_cast<Workspace>(ws)};
    AnalysisDataService::Instance().addOrReplace(wsname, ws2);
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
                     11);
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

private:
  std::string m_filename;
  std::string long_filename;
  std::string m_name;
};
#endif /*MANTID_DATAHANDLING_SAVEMFTTEST_H_*/
