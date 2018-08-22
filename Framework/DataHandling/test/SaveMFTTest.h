#ifndef MANTID_DATAHANDLING_SAVEMFTTEST_H_
#define MANTID_DATAHANDLING_SAVEMFTTEST_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/SaveMFT.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/PropertyWithValue.h"
#include <Poco/File.h>
#include <Poco/TemporaryFile.h>
#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>
#include <iterator>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class SaveMFTTest : public CxxTest::TestSuite {

public:
  static SaveMFTTest *createSuite() { return new SaveMFTTest(); }
  static void destroySuite(SaveMFTTest *suite) { delete suite; }

  SaveMFTTest() {}
  ~SaveMFTTest() override {}

  void testInit() {
    SaveMFT alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized())
  }

  void test_invalid_InputWorkspace() {
    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", "ws"))
    TS_ASSERT_THROWS(alg.setProperty("InputWorkspace", "abc"),
                     std::invalid_argument)
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error)
    TS_ASSERT(!alg.isExecuted())
  }

  void test_point_data() {
    const auto &x1 = Mantid::HistogramData::Points({0.33, 0.34});
    const auto &y1 = Mantid::HistogramData::Counts({3., 6.6});
    Mantid::HistogramData::Histogram histogram(x1, y1);
    const Workspace_sptr ws = create<Workspace2D>(1, histogram);
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string file = outputFileHandle.path();
    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted());
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename).exists())
    std::vector<std::string> data;
    data.reserve(5);
    data.emplace_back("MFT");
    data.emplace_back("");
    data.emplace_back("                           q                        "
                      "refl                    refl_err");
    data.emplace_back("       3.300000000000000e-01       "
                      "3.000000000000000e+00       1.732050807568877e+00");
    data.emplace_back("       3.400000000000000e-01       "
                      "6.600000000000000e+00       2.569046515733026e+00");
    std::ifstream in(filename);
    TS_ASSERT(not_empty(in))
    std::string fullline;
    auto it = data.begin();
    while (std::getline(in, fullline)) {
      if (fullline.find(" : ") == std::string::npos)
        TS_ASSERT_EQUALS(fullline, *(it++));
    }
    TS_ASSERT(in.eof())
  }

  void test_histogram_data() {
    const auto &x1 = Mantid::HistogramData::BinEdges({2.4, 3.7, 10.8});
    const auto &y1 = Mantid::HistogramData::Counts({3., 6.6});
    Mantid::HistogramData::Histogram histogram(x1, y1);
    Workspace2D_sptr ws = create<Workspace2D>(1, histogram);
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string file = outputFileHandle.path();
    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted());
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename).exists());
    std::vector<std::string> data;
    data.reserve(5);
    data.emplace_back("MFT");
    data.emplace_back("");
    data.emplace_back("                           q                        "
                      "refl                    refl_err");
    data.emplace_back("       3.050000000000000e+00       "
                      "3.000000000000000e+00       1.732050807568877e+00");
    data.emplace_back("       7.250000000000000e+00       "
                      "6.600000000000000e+00       2.569046515733026e+00");
    std::ifstream in(filename);
    std::string fullline;
    auto it = data.begin();
    TS_ASSERT(not_empty(in))
    while (std::getline(in, fullline)) {
      if (fullline.find(" : ") == std::string::npos)
        TS_ASSERT_EQUALS(fullline, *(it++))
    }
    TS_ASSERT(in.eof())
  }

  void test_empty_workspace() {
    auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string file = outputFileHandle.path();
    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_ANYTHING(alg.execute())
    TS_ASSERT(!alg.isExecuted())
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(!Poco::File(filename).exists())
  }

  void test_number_lines_for_two_data_values() {
    const auto &x1 = Mantid::HistogramData::Points({0.33, 0.34});
    const auto &y1 = Mantid::HistogramData::Counts({3., 6.6});
    Mantid::HistogramData::Histogram histogram(x1, y1);
    auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();
    ws->initialize(1, histogram);
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string file = outputFileHandle.path();
    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename).exists())
    std::ifstream in(filename);
    // Total number of lines
    TS_ASSERT(not_empty(in))
    TS_ASSERT_EQUALS(std::count(std::istreambuf_iterator<char>(in),
                                std::istreambuf_iterator<char>(),
                                in.widen('\n')),
                     25)
  }

  void test_dx_values() {
    const auto &x1 = Mantid::HistogramData::Points({0.33, 0.34});
    const auto &y1 = Mantid::HistogramData::Counts({3., 6.6});
    Mantid::HistogramData::Histogram histogram(x1, y1);
    histogram.setPointStandardDeviations(std::vector<double>{1.1, 1.3});
    const Workspace_sptr ws = create<Workspace2D>(1, histogram);
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string file = outputFileHandle.path();
    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted());
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename).exists())
    std::vector<std::string> data;
    data.reserve(5);
    data.emplace_back("MFT");
    data.emplace_back("");
    data.emplace_back("                           q                        "
                      "refl                    refl_err                q_res "
                      "(FWHM)");
    data.emplace_back("       3.300000000000000e-01       "
                      "3.000000000000000e+00       1.732050807568877e+00       "
                      "1.100000000000000e+00");
    data.emplace_back("       3.400000000000000e-01       "
                      "6.600000000000000e+00       2.569046515733026e+00       "
                      "1.300000000000000e+00");
    std::ifstream in(filename);
    TS_ASSERT(not_empty(in))
    std::string fullline;
    auto it = data.begin();
    while (std::getline(in, fullline)) {
      if (fullline.find(" : ") == std::string::npos)
        TS_ASSERT_EQUALS(fullline, *(it++))
    }
  }

  void test_no_header() {
    const auto &x1 = Mantid::HistogramData::Points({0.33, 0.34});
    const auto &y1 = Mantid::HistogramData::Counts({3., 6.6});
    Mantid::HistogramData::Histogram histogram(x1, y1);
    const Workspace_sptr ws = create<Workspace2D>(1, histogram);
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string file = outputFileHandle.path();
    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WriteHeader", false))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename).exists())
    std::vector<std::string> data;
    data.reserve(2);
    data.emplace_back("       3.300000000000000e-01       "
                      "3.000000000000000e+00       1.732050807568877e+00");
    data.emplace_back("       3.400000000000000e-01       "
                      "6.600000000000000e+00       2.569046515733026e+00");
    std::ifstream in(filename);
    TS_ASSERT(not_empty(in))
    std::string fullline;
    auto it = data.begin();
    while (std::getline(in, fullline)) {
      TS_ASSERT_EQUALS(fullline, *(it++))
    }
    TS_ASSERT(in.eof())
  }

  void test_override_existing_file() {
    const auto &x1 = Mantid::HistogramData::Points({4.36, 6.32});
    const auto &y1 = Mantid::HistogramData::Counts({4., 7.6});
    Mantid::HistogramData::Histogram histogram1(x1, y1);
    const Workspace_sptr ws1 = create<Workspace2D>(1, histogram1);
    const auto &x2 = Mantid::HistogramData::Points({0.33, 0.34});
    const auto &y2 = Mantid::HistogramData::Counts({3., 6.6});
    Mantid::HistogramData::Histogram histogram2(x2, y2);
    const Workspace_sptr ws2 = create<Workspace2D>(1, histogram2);
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string file = outputFileHandle.path();
    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws1))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WriteHeader", false))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws2))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WriteHeader", false))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename).exists())
    std::vector<std::string> data;
    data.reserve(2);
    data.emplace_back("       3.300000000000000e-01       "
                      "3.000000000000000e+00       1.732050807568877e+00");
    data.emplace_back("       3.400000000000000e-01       "
                      "6.600000000000000e+00       2.569046515733026e+00");
    std::ifstream in(filename);
    TS_ASSERT(not_empty(in))
    std::string fullline;
    auto it = data.begin();
    while (std::getline(in, fullline)) {
      TS_ASSERT_EQUALS(fullline, *(it++));
    }
    TS_ASSERT(in.eof())
  }

  void test_more_than_nine_logs() {
    const auto &x1 = Mantid::HistogramData::Points({0.33, 0.34});
    const auto &y1 = Mantid::HistogramData::Counts({3., 6.6});
    Mantid::HistogramData::Histogram histogram(x1, y1);
    auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();
    ws->initialize(1, histogram);
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string file = outputFileHandle.path();
    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("LogList", std::vector<std::string>{"a", "b"}))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename).exists())
    std::ifstream in(filename);
    std::string line;
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "MFT")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Instrument : Not defined")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "User-local contact : Not defined")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Title : Not defined")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Subtitle : Not defined")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Start date + time : Not defined")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "End date + time : Not defined")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Theta 1 + dir + ref numbers : Not defined")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Theta 2 + dir + ref numbers : Not defined")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Theta 3 + dir + ref numbers : Not defined")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "a : Not defined")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "b : Not defined")
    for (int i = 0; i < 7; ++i) {
      std::getline(in, line);
      TS_ASSERT_EQUALS(line, "Parameter  : Not defined")
    }
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Number of file format : 40")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Number of data points : 2")
  }

  void test_defined_log() {
    const auto &x1 = Mantid::HistogramData::Points({0.33, 0.34});
    const auto &y1 = Mantid::HistogramData::Counts({3., 6.6});
    Mantid::HistogramData::Histogram histogram(x1, y1);
    auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();
    ws->initialize(1, histogram);
    Mantid::Kernel::PropertyWithValue<int> *a =
        new Mantid::Kernel::PropertyWithValue<int>("a", 5);
    ws->mutableRun().addLogData(a);
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string file = outputFileHandle.path();
    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("LogList", std::vector<std::string>{"a"}))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename).exists())
    std::ifstream in(filename);
    std::string line;
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "MFT")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Instrument : Not defined")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "User-local contact : Not defined")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Title : Not defined")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Subtitle : Not defined")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Start date + time : Not defined")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "End date + time : Not defined")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Theta 1 + dir + ref numbers : Not defined")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Theta 2 + dir + ref numbers : Not defined")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Theta 3 + dir + ref numbers : Not defined")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "a : 5")
    for (int i = 0; i < 8; ++i) {
      std::getline(in, line);
      TS_ASSERT_EQUALS(line, "Parameter  : Not defined")
    }
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Number of file format : 40")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Number of data points : 2")
  }

  void test_group_workspaces() {
    const auto &x1 = Mantid::HistogramData::Points({4.36, 6.32});
    const auto &y1 = Mantid::HistogramData::Counts({4., 7.6});
    Mantid::HistogramData::Histogram histogram1(x1, y1);
    const Workspace_sptr ws1 = create<Workspace2D>(1, histogram1);
    const auto &x2 = Mantid::HistogramData::Points({0.33, 0.34});
    const auto &y2 = Mantid::HistogramData::Counts({3., 6.6});
    Mantid::HistogramData::Histogram histogram2(x2, y2);
    const Workspace_sptr ws2 = create<Workspace2D>(1, histogram2);
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string file = outputFileHandle.path();
    WorkspaceGroup_sptr group(new WorkspaceGroup());
    group->add("ws1");
    group->add("ws2");
    SaveMFT alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", group))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WriteHeader", false))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    const std::string filename = alg.getPropertyValue("Filename");
    std::string f1 = filename.substr(0, filename.find(".")) + "ws1";
    std::string f2 = filename.substr(0, filename.find(".")) + "ws2";
    TS_ASSERT(Poco::File(f1).exists())
    TS_ASSERT(Poco::File(f2).exists())
  }

private:
  bool not_empty(std::ifstream &in) {
    return in.peek() != std::ifstream::traits_type::eof();
  }
};
#endif /*MANTID_DATAHANDLING_SAVEMFTTEST_H_*/
