// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAVEReflectometryAsciiTEST_H_
#define MANTID_DATAHANDLING_SAVEReflectometryAsciiTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/SaveReflectometryAscii.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/PropertyWithValue.h"
#include <Poco/File.h>
#include <Poco/TemporaryFile.h>
#include <boost/make_shared.hpp>
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <iterator>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class SaveReflectometryAsciiTest : public CxxTest::TestSuite {

public:
  static SaveReflectometryAsciiTest *createSuite() {
    return new SaveReflectometryAsciiTest();
  }
  static void destroySuite(SaveReflectometryAsciiTest *suite) { delete suite; }

  SaveReflectometryAsciiTest() {}
  ~SaveReflectometryAsciiTest() override {}

  void testInit() {
    SaveReflectometryAscii alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized())
  }

  void test_invalid_InputWorkspace() {
    SaveReflectometryAscii alg;
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
    SaveReflectometryAscii alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted());
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename.append(".mft")).exists())
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
    SaveReflectometryAscii alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted());
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename.append(".mft")).exists())
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
    SaveReflectometryAscii alg;
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
    SaveReflectometryAscii alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename.append(".mft")).exists())
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
    SaveReflectometryAscii alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted());
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename.append(".mft")).exists())
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

  void test_txt() {
    const auto &x1 = Mantid::HistogramData::Points({0.33, 0.34});
    const auto &y1 = Mantid::HistogramData::Counts({3., 6.6});
    Mantid::HistogramData::Histogram histogram(x1, y1);
    const Workspace_sptr ws = create<Workspace2D>(1, histogram);
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string file = outputFileHandle.path();
    SaveReflectometryAscii alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FileExtension", ".txt"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename.append(".txt")).exists())
    std::vector<std::string> data;
    data.reserve(2);
    data.emplace_back("\t3.300000000000000e-01\t"
                      "3.000000000000000e+00\t1.732050807568877e+00\t"
                      "6.502941176470588e-01");
    data.emplace_back("\t3.400000000000000e-01\t"
                      "6.600000000000000e+00\t2.569046515733026e+00\t"
                      "6.700000000000000e-01");
    std::ifstream in(filename);
    TS_ASSERT(not_empty(in))
    std::string fullline;
    auto it = data.begin();
    while (std::getline(in, fullline)) {
      TS_ASSERT_EQUALS(fullline, *(it++))
    }
    TS_ASSERT(in.eof())
  }

  void test_override_existing_file_txt() {
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
    SaveReflectometryAscii alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws1))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FileExtension", ".txt"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws2))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FileExtension", ".txt"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename.append(".txt")).exists())
    std::vector<std::string> data;
    data.reserve(2);
    data.emplace_back("\t3.300000000000000e-01\t"
                      "3.000000000000000e+00\t1.732050807568877e+00\t"
                      "6.502941176470588e-01");
    data.emplace_back("\t3.400000000000000e-01\t"
                      "6.600000000000000e+00\t2.569046515733026e+00\t"
                      "6.700000000000000e-01");
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
    SaveReflectometryAscii alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("LogList", std::vector<std::string>{"a", "b"}))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename.append(".mft")).exists())
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

  void test_user_log() {
    const auto &x1 = Mantid::HistogramData::Points({0.33, 0.34});
    const auto &y1 = Mantid::HistogramData::Counts({3., 6.6});
    Mantid::HistogramData::Histogram histogram(x1, y1);
    auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();
    ws->initialize(1, histogram);
    Mantid::Kernel::PropertyWithValue<int> *a =
        new Mantid::Kernel::PropertyWithValue<int>("a", 5);
    ws->mutableRun().addLogData(a);
    Mantid::Kernel::PropertyWithValue<double> *b =
        new Mantid::Kernel::PropertyWithValue<double>("b", 3.4382);
    ws->mutableRun().addLogData(b);
    ws->mutableRun().getProperty("b")->setUnits("MyUnit");
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string file = outputFileHandle.path();
    SaveReflectometryAscii alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogList", "a, b"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename.append(".mft")).exists())
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
    TS_ASSERT_EQUALS(line, "a : 5 ")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "b : 3.4382000000000001 MyUnit")
    for (int i = 0; i < 7; ++i) {
      std::getline(in, line);
      TS_ASSERT_EQUALS(line, "Parameter  : Not defined")
    }
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Number of file format : 40")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Number of data points : 2")
  }

  void test_user_log_overrides_fixed_log() {
    const auto &x1 = Mantid::HistogramData::Points({0.33, 0.34});
    const auto &y1 = Mantid::HistogramData::Counts({3., 6.6});
    Mantid::HistogramData::Histogram histogram(x1, y1);
    auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();
    ws->initialize(1, histogram);
    // User wants to add the Instrument name header line
    Mantid::Kernel::PropertyWithValue<std::string> *a =
        new Mantid::Kernel::PropertyWithValue<std::string>("Instrument", "ABC");
    ws->mutableRun().addLogData(a);
    // The workspace has an entry already for the instrument name
    Mantid::Kernel::PropertyWithValue<std::string> *b =
        new Mantid::Kernel::PropertyWithValue<std::string>("instrument.name",
                                                           "DEF");
    ws->mutableRun().addLogData(b);
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string file = outputFileHandle.path();
    SaveReflectometryAscii alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("LogList", "Instrument, instrument.name"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename.append(".mft")).exists())
    std::ifstream in(filename);
    std::string line;
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "MFT")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Instrument : DEF ")
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
    TS_ASSERT_EQUALS(line, "Instrument : ABC ")
    for (int i = 0; i < 8; ++i) {
      std::getline(in, line);
      TS_ASSERT_EQUALS(line, "Parameter  : Not defined")
    }
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Number of file format : 40")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Number of data points : 2")
  }

  void test_automatic_log_filling() {
    const auto &x1 = Mantid::HistogramData::Points({0.33, 0.34});
    const auto &y1 = Mantid::HistogramData::Counts({3., 6.6});
    Mantid::HistogramData::Histogram histogram(x1, y1);
    auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();
    ws->initialize(1, histogram);
    // Should use this instrument name
    Mantid::Kernel::PropertyWithValue<std::string> *a =
        new Mantid::Kernel::PropertyWithValue<std::string>("instrument.name",
                                                           "DEF");
    ws->mutableRun().addLogData(a);
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string file = outputFileHandle.path();
    SaveReflectometryAscii alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename.append(".mft")).exists())
    std::ifstream in(filename);
    std::string line;
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "MFT")
    std::getline(in, line);
    TS_ASSERT_EQUALS(line, "Instrument : DEF ")
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
    for (int i = 0; i < 9; ++i) {
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
    AnalysisDataService::Instance().addOrReplace("ws1", ws1);
    AnalysisDataService::Instance().addOrReplace("ws2", ws2);
    const std::string file = outputFileHandle.path();
    WorkspaceGroup_sptr group(new WorkspaceGroup());
    AnalysisDataService::Instance().addOrReplace("group", group);
    group->add("ws1");
    group->add("ws2");
    SaveReflectometryAscii alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "group"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FileExtension", ".txt"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    const std::string filename = alg.getPropertyValue("Filename");
    std::string f1 = filename + "ws1.txt";
    std::string f2 = filename + "ws2.txt";
    TS_ASSERT(Poco::File(f1).exists())
    std::vector<std::string> data1;
    data1.reserve(2);
    data1.emplace_back("\t4.360000000000000e+00\t"
                       "4.000000000000000e+00\t2.000000000000000e+00\t"
                       "7.367848101265823e+00");
    data1.emplace_back("\t6.320000000000000e+00\t"
                       "7.600000000000000e+00\t2.756809750418044e+00\t"
                       "1.068000000000000e+01");
    std::ifstream in1(f1);
    TS_ASSERT(not_empty(in1))
    std::string fullline;
    auto it1 = data1.begin();
    while (std::getline(in1, fullline)) {
      TS_ASSERT_EQUALS(fullline, *(it1++));
    }
    TS_ASSERT(in1.eof())
    TS_ASSERT(Poco::File(f2).exists())
    std::vector<std::string> data2;
    data2.reserve(2);
    data2.emplace_back("\t3.300000000000000e-01\t"
                       "3.000000000000000e+00\t1.732050807568877e+00\t"
                       "6.502941176470588e-01");
    data2.emplace_back("\t3.400000000000000e-01\t"
                       "6.600000000000000e+00\t2.569046515733026e+00\t"
                       "6.700000000000000e-01");
    std::ifstream in2(f2);
    TS_ASSERT(not_empty(in2))
    auto it2 = data2.begin();
    while (std::getline(in2, fullline)) {
      TS_ASSERT_EQUALS(fullline, *(it2++));
    }
    TS_ASSERT(in2.eof())
  }

  void test_point_data_dat() {
    const auto &x1 = Mantid::HistogramData::Points({0.33, 0.34});
    const auto &y1 = Mantid::HistogramData::Counts({3., 6.6});
    Mantid::HistogramData::Histogram histogram(x1, y1);
    const Workspace_sptr ws = create<Workspace2D>(1, histogram);
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string file = outputFileHandle.path();
    SaveReflectometryAscii alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FileExtension", ".dat"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted());
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename.append(".dat")).exists())
    std::vector<std::string> data;
    data.reserve(3);
    data.emplace_back("2");
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

  void test_dx_values_with_header_custom() {
    const auto &x1 = Mantid::HistogramData::Points({0.33, 0.34});
    const auto &y1 = Mantid::HistogramData::Counts({3., 6.6});
    Mantid::HistogramData::Histogram histogram(x1, y1);
    histogram.setPointStandardDeviations(std::vector<double>{1.1, 1.3});
    const Workspace_sptr ws = create<Workspace2D>(1, histogram);
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string file = outputFileHandle.path();
    SaveReflectometryAscii alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FileExtension", "custom"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WriteHeader", true))
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
    data.emplace_back("\t3.300000000000000e-01\t"
                      "3.000000000000000e+00\t1.732050807568877e+00\t"
                      "1.100000000000000e+00");
    data.emplace_back("\t3.400000000000000e-01\t"
                      "6.600000000000000e+00\t2.569046515733026e+00\t"
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

  void test_dx_values_no_header_custom() {
    const auto &x1 = Mantid::HistogramData::Points({0.33, 0.34});
    const auto &y1 = Mantid::HistogramData::Counts({3., 6.6});
    Mantid::HistogramData::Histogram histogram(x1, y1);
    histogram.setPointStandardDeviations(std::vector<double>{1.1, 1.3});
    const Workspace_sptr ws = create<Workspace2D>(1, histogram);
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string file = outputFileHandle.path();
    SaveReflectometryAscii alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FileExtension", "custom"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WriteHeader", false))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted());
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename).exists())
    std::vector<std::string> data;
    data.reserve(2);
    data.emplace_back("\t3.300000000000000e-01\t"
                      "3.000000000000000e+00\t1.732050807568877e+00\t"
                      "1.100000000000000e+00");
    data.emplace_back("\t3.400000000000000e-01\t"
                      "6.600000000000000e+00\t2.569046515733026e+00\t"
                      "1.300000000000000e+00");
    std::ifstream in(filename);
    TS_ASSERT(not_empty(in))
    std::string fullline;
    auto it = data.begin();
    while (std::getline(in, fullline)) {
      TS_ASSERT_EQUALS(fullline, *(it++))
    }
  }

  void test_no_header_no_resolution_separator_custom() {
    const auto &x1 = Mantid::HistogramData::Points({0.33, 0.34});
    const auto &y1 = Mantid::HistogramData::Counts({3., 6.6});
    Mantid::HistogramData::Histogram histogram(x1, y1);
    histogram.setPointStandardDeviations(std::vector<double>{1.1, 1.3});
    const Workspace_sptr ws = create<Workspace2D>(1, histogram);
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string file = outputFileHandle.path();
    SaveReflectometryAscii alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", file))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FileExtension", "custom"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WriteHeader", false))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WriteResolution", false))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Separator", "space"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted());
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename).exists())
    std::vector<std::string> data;
    data.reserve(2);
    data.emplace_back(" 3.300000000000000e-01 "
                      "3.000000000000000e+00 1.732050807568877e+00");
    data.emplace_back(" 3.400000000000000e-01 "
                      "6.600000000000000e+00 2.569046515733026e+00");
    std::ifstream in(filename);
    TS_ASSERT(not_empty(in))
    std::string fullline;
    auto it = data.begin();
    while (std::getline(in, fullline)) {
      TS_ASSERT_EQUALS(fullline, *(it++))
    }
  }

private:
  bool not_empty(std::ifstream &in) {
    return in.peek() != std::ifstream::traits_type::eof();
  }
};
#endif /*MANTID_DATAHANDLING_SAVEReflectometryAsciiTEST_H_*/
