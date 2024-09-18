// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include <fstream>
#include <numeric>

#include "MantidKernel/LogParser.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <boost/lexical_cast.hpp>

#include <filesystem>

using namespace std;
using namespace Mantid::Kernel;
using namespace Mantid::Types::Core;

class LogParserTest : public CxxTest::TestSuite {
public:
  static LogParserTest *createSuite() { return new LogParserTest(); }
  static void destroySuite(LogParserTest *suite) { delete suite; }

  class TmpFile {
    filesystem::path m_file;

  public:
    TmpFile(const std::string &fname) : m_file(fname) {}
    ~TmpFile() { remove(); }
    const std::string path() const { return m_file.string(); }
    bool exists() const { return filesystem::exists(m_file); }
    void remove() {
      if (filesystem::exists(m_file))
        filesystem::remove(m_file);
    }
  };

  LogParserTest()
      : log_num_good("TST000000_good.txt"), log_num_late("TST000000_late.txt"), log_num_early("TST000000_early.txt"),
        log_num_single("TST000000_single.txt"), log_str("TST000000_str.txt"), icp_file("TST000000_icpevent.txt"),
        log_str_repeat("TST000000_repeat.txt"), log_num_repeat("TST000000_num_repeat.txt"),
        log_str_continuations("TST000000_str_continue.txt") {}

  void testGood() {
    mkICP();
    mkGood();
    const auto &icp_log = std::unique_ptr<Property>(LogParser::createLogProperty(icp_file.path(), "icpevent"));
    const LogParser lp(icp_log.get());
    const auto &p1 = std::unique_ptr<Property>(lp.createLogProperty(log_num_good.path(), "good"));
    TS_ASSERT(p1);
    auto *tp1 = dynamic_cast<TimeSeriesProperty<double> *>(p1.get());
    std::map<DateAndTime, double> vmap = tp1->valueAsMap();
    std::map<DateAndTime, double>::iterator v = vmap.begin();
    // time 1
    TS_ASSERT_EQUALS(vmap.size(), 9);
    TS_ASSERT_EQUALS(v->second, 1);
    const auto &ti_data1 = v->first.to_tm();
    TS_ASSERT_EQUALS(ti_data1.tm_hour, 12);
    TS_ASSERT_EQUALS(ti_data1.tm_min, 22);
    ++v;
    ++v;
    ++v;
    ++v;
    // time 5
    const auto &ti_data5 = v->first.to_tm();
    TS_ASSERT_EQUALS(ti_data5.tm_hour, 12);
    TS_ASSERT_EQUALS(ti_data5.tm_min, 22);
    // last time
    std::map<DateAndTime, double>::reverse_iterator rv = vmap.rbegin();
    TS_ASSERT_EQUALS(rv->second, 9);
    const auto &ti_data_last = rv->first.to_tm();
    TS_ASSERT_EQUALS(ti_data_last.tm_hour, 14);
    TS_ASSERT_EQUALS(ti_data_last.tm_min, 3);
    TS_ASSERT_DELTA(timeMean(p1.get()), 8.4904, 0.001);

    TS_ASSERT_EQUALS(tp1->nthValue(0), 1);
    TS_ASSERT_EQUALS(tp1->nthValue(1), 2);
    TS_ASSERT_EQUALS(tp1->nthValue(2), 3);
    TS_ASSERT_EQUALS(tp1->nthValue(3), 4);
    TS_ASSERT_EQUALS(tp1->nthValue(4), 5);
    TS_ASSERT_EQUALS(tp1->nthValue(5), 6);
    TS_ASSERT_EQUALS(tp1->nthValue(6), 7);
    TS_ASSERT_EQUALS(tp1->nthValue(7), 8);

    TS_ASSERT_EQUALS(tp1->firstValue(), 1);
    TS_ASSERT_EQUALS(tp1->lastValue(), 9);
  }

  void testLate() {
    mkICP();
    mkLate();
    const auto &icp_log = std::unique_ptr<Property>(LogParser::createLogProperty(icp_file.path(), "icpevent"));
    const LogParser lp(icp_log.get());
    const auto &p1 = std::unique_ptr<Property>(lp.createLogProperty(log_num_late.path(), "late"));
    TS_ASSERT(p1);
    auto *tp1 = dynamic_cast<TimeSeriesProperty<double> *>(p1.get());
    std::map<DateAndTime, double> vmap = tp1->valueAsMap();
    std::map<DateAndTime, double>::iterator v = vmap.begin();

    // time 1
    TS_ASSERT_EQUALS(vmap.size(), 8);
    TS_ASSERT_EQUALS(v->second, 2);

    const auto &ti_data1 = v->first.to_tm();
    TS_ASSERT_EQUALS(ti_data1.tm_hour, 12);
    TS_ASSERT_EQUALS(ti_data1.tm_min, 22);
    ++v;
    ++v;
    ++v;
    ++v;
    // time 5
    const auto &ti_data5 = v->first.to_tm();
    TS_ASSERT_EQUALS(ti_data5.tm_hour, 12);
    TS_ASSERT_EQUALS(ti_data5.tm_min, 22);
    // last time
    std::map<DateAndTime, double>::reverse_iterator rv = vmap.rbegin();
    TS_ASSERT_EQUALS(rv->second, 9);
    const auto &ti_dataLast = rv->first.to_tm();
    TS_ASSERT_EQUALS(ti_dataLast.tm_hour, 14);
    TS_ASSERT_EQUALS(ti_dataLast.tm_min, 3);
    TS_ASSERT_DELTA(timeMean(p1.get()), 8.4941, 0.001);
  }

  void testEarly() {
    mkICP();
    mkEarly();
    const auto &icp_log = std::unique_ptr<Property>(LogParser::createLogProperty(icp_file.path(), "icpevent"));
    const LogParser lp(icp_log.get());
    const auto &p1 = std::unique_ptr<Property>(lp.createLogProperty(log_num_early.path(), "early"));
    TS_ASSERT(p1);
    auto *tp1 = dynamic_cast<TimeSeriesProperty<double> *>(p1.get());
    std::map<DateAndTime, double> vmap = tp1->valueAsMap();
    std::map<DateAndTime, double>::iterator v = vmap.begin();

    // time 1
    TS_ASSERT_EQUALS(vmap.size(), 8);
    TS_ASSERT_EQUALS(v->second, 1);
    const auto &ti_data1 = v->first.to_tm();
    TS_ASSERT_EQUALS(ti_data1.tm_hour, 12);
    TS_ASSERT_EQUALS(ti_data1.tm_min, 22);
    ++v;
    ++v;
    ++v;
    ++v;
    // time 5
    const auto &ti_data5 = v->first.to_tm();
    TS_ASSERT_EQUALS(ti_data5.tm_hour, 12);
    TS_ASSERT_EQUALS(ti_data5.tm_min, 22);
    // last time
    std::map<DateAndTime, double>::reverse_iterator rv = vmap.rbegin();
    TS_ASSERT_EQUALS(rv->second, 8);
    const auto &ti_dataLast = rv->first.to_tm();
    TS_ASSERT_EQUALS(ti_dataLast.tm_hour, 12);
    TS_ASSERT_EQUALS(ti_dataLast.tm_min, 23);
    TS_ASSERT_DELTA(timeMean(p1.get()), 4.9090, 0.001);
  }

  void testSingle() {
    mkICP();
    mkSingle();
    const auto &icp_log = std::unique_ptr<Property>(LogParser::createLogProperty(icp_file.path(), "icpevent"));
    const LogParser lp(icp_log.get());
    const auto &p1 = std::unique_ptr<Property>(lp.createLogProperty(log_num_single.path(), "single"));
    TS_ASSERT(p1);
    auto *tp1 = dynamic_cast<TimeSeriesProperty<double> *>(p1.get());
    std::map<DateAndTime, double> vmap = tp1->valueAsMap();
    std::map<DateAndTime, double>::iterator v = vmap.begin();

    // time 1
    TS_ASSERT_EQUALS(vmap.size(), 1);
    TS_ASSERT_EQUALS(v->second, 4);
    const auto &ti_data1 = v->first.to_tm();
    TS_ASSERT_EQUALS(ti_data1.tm_hour, 12);
    TS_ASSERT_EQUALS(ti_data1.tm_min, 22);
    // Can't get a valid mean with a single time and no intervals in it.
    // TS_ASSERT_DELTA(timeMean(p1),4., 0.001);
  }

  void testStr() {
    mkICP();
    mkStr();
    const auto &icp_log = std::unique_ptr<Property>(LogParser::createLogProperty(icp_file.path(), "icpevent"));
    const LogParser lp(icp_log.get());
    const auto &p1 = std::unique_ptr<Property>(lp.createLogProperty(log_str.path(), "str"));
    TS_ASSERT(p1);
    auto *tp1 = dynamic_cast<TimeSeriesProperty<std::string> *>(p1.get());
    std::map<DateAndTime, std::string> vmap = tp1->valueAsMap();
    std::map<DateAndTime, std::string>::iterator v = vmap.begin();
    // time 1
    TS_ASSERT_EQUALS(vmap.size(), 9);
    TS_ASSERT_EQUALS(v->second, "   line 1");
    const auto &ti_data1 = v->first.to_tm();
    TS_ASSERT_EQUALS(ti_data1.tm_hour, 12);
    TS_ASSERT_EQUALS(ti_data1.tm_min, 22);
    ++v;
    ++v;
    ++v;
    // time 4
    TS_ASSERT_EQUALS(v->second, "   line 4");
    const auto &ti_data4 = v->first.to_tm();
    TS_ASSERT_EQUALS(ti_data4.tm_hour, 12);
    TS_ASSERT_EQUALS(ti_data4.tm_min, 22);
    // last time
    std::map<DateAndTime, std::string>::reverse_iterator rv = vmap.rbegin();
    TS_ASSERT_EQUALS(rv->second, "   line 9");
    const auto &ti_dataLast = rv->first.to_tm();
    TS_ASSERT_EQUALS(ti_dataLast.tm_hour, 14);
    TS_ASSERT_EQUALS(ti_dataLast.tm_min, 3);
    TS_ASSERT_THROWS(timeMean(p1.get()), const std::runtime_error &);
  }

  // Test a variant of the log file containing CHANGE_PERIOD flags
  void testConstructionFromFileUsingICPVariant_CHANGE_PERIOD() {
    mkICPVariant();
    const auto &icp_log = std::unique_ptr<Property>(LogParser::createLogProperty(icp_file.path(), "icpevent"));
    const LogParser lp(icp_log.get());
    const auto &prop = std::unique_ptr<Property>(lp.createAllPeriodsLog());
    const auto *timeseriesprop = dynamic_cast<const TimeSeriesProperty<int> *>(prop.get());
    TS_ASSERT(timeseriesprop);
    // Check the size
    TS_ASSERT_EQUALS(4, timeseriesprop->size());
    // Check the exact time stamps
    TS_ASSERT_EQUALS(DateAndTime("2000-09-05T12:22:55"), timeseriesprop->nthTime(0));
    TS_ASSERT_EQUALS(DateAndTime("2000-09-05T12:23:08"), timeseriesprop->nthTime(1));
    TS_ASSERT_EQUALS(DateAndTime("2000-09-05T12:23:22"), timeseriesprop->nthTime(2));
    TS_ASSERT_EQUALS(DateAndTime("2000-09-05T12:23:37"), timeseriesprop->nthTime(3));
  }

  void testConstructionFromPropertyUsingICPVariant_CHANGE_PERIOD() {
    auto log = std::make_unique<TimeSeriesProperty<std::string>>("ICPLog");
    // Notice we are using "CHANGE_PERIOD"
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:15:00", "CHANGE_PERIOD 1"));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:16:00", "CHANGE_PERIOD 2"));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:00", "CHANGE_PERIOD 3"));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:18:00", "CHANGE_PERIOD 2"));

    const LogParser logparser(log.get());

    const auto &prop = std::unique_ptr<Property>(logparser.createAllPeriodsLog());
    const auto *timeseriesprop = dynamic_cast<const TimeSeriesProperty<int> *>(prop.get());
    TS_ASSERT(timeseriesprop);
    // Check the size
    TS_ASSERT_EQUALS(4, timeseriesprop->size());
    // Check the exact time stamps
    TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:15:00"), timeseriesprop->nthTime(0));
    TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:16:00"), timeseriesprop->nthTime(1));
    TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:17:00"), timeseriesprop->nthTime(2));
    TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:18:00"), timeseriesprop->nthTime(3));
  }

  void testConstructionFromPropertyUsingICPVariant_CHANGE_SPACE_PERIOD() {
    auto log = std::make_unique<TimeSeriesProperty<std::string>>("ICPLog");
    // Notice we are using "CHANGE PERIOD"
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:15:00", "CHANGE PERIOD 1"));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:16:00", "CHANGE PERIOD 2"));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:00", "CHANGE PERIOD 3"));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:18:00", "CHANGE PERIOD 2"));

    const LogParser logparser(log.get());

    const auto &prop = std::unique_ptr<Property>(logparser.createAllPeriodsLog());
    const auto *timeseriesprop = dynamic_cast<const TimeSeriesProperty<int> *>(prop.get());
    TS_ASSERT(timeseriesprop);
    // Check the size
    TS_ASSERT_EQUALS(4, timeseriesprop->size());
    // Check the exact time stamps
    TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:15:00"), timeseriesprop->nthTime(0));
    TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:16:00"), timeseriesprop->nthTime(1));
    TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:17:00"), timeseriesprop->nthTime(2));
    TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:18:00"), timeseriesprop->nthTime(3));
  }

  // Check that periods that don't have a full "CHANGE PERIOD" flag are not
  // added.
  void testWontAddPeriodWithoutPERIODpartOfCHANGE_SPACE_PERIOD() {
    auto log = std::make_unique<TimeSeriesProperty<std::string>>("ICPLog");
    // Notice we are using "CHANGE PERIOD"
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:15:00", "CHANGE PERIOD 1"));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:16:00", "CHANGE PERIOD 2"));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:00", "CHANGE PERIOD 3"));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:18:00",
                                           "CHANGE 2")); // This is a duff entry. Shouldn't get added.

    const LogParser logparser(log.get());

    const auto &prop = std::unique_ptr<Property>(logparser.createAllPeriodsLog());
    const auto *timeseriesprop = dynamic_cast<const TimeSeriesProperty<int> *>(prop.get());
    TS_ASSERT(timeseriesprop);
    // Check the size
    TS_ASSERT_EQUALS(3, timeseriesprop->size());
    // Check the exact time stamps
    TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:15:00"), timeseriesprop->nthTime(0));
    TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:16:00"), timeseriesprop->nthTime(1));
    TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:17:00"), timeseriesprop->nthTime(2));
  }

  void testCreatesCurrentPeriodLog() {
    // Check it with a few expected period numbers.
    doTestCurrentPeriodLog(1);
    doTestCurrentPeriodLog(2);
    doTestCurrentPeriodLog(3);
  }

  void testNoICPevent() {
    if (icp_file.exists())
      icp_file.remove();
    mkGood();
    const auto &icp_log = std::unique_ptr<Property>(LogParser::createLogProperty(icp_file.path(), "icpevent"));
    const LogParser lp(icp_log.get());
    const auto &p1 = std::unique_ptr<Property>(lp.createLogProperty(log_num_good.path(), "good"));
    TS_ASSERT(p1);
    auto *tp1 = dynamic_cast<TimeSeriesProperty<double> *>(p1.get());
    std::map<DateAndTime, double> vmap = tp1->valueAsMap();
    std::map<DateAndTime, double>::iterator v = vmap.begin();

    // time 1
    TS_ASSERT_EQUALS(vmap.size(), 9);
    TS_ASSERT_EQUALS(v->second, 1);
    const auto &ti_data1 = v->first.to_tm();
    TS_ASSERT_EQUALS(ti_data1.tm_hour, 12);
    TS_ASSERT_EQUALS(ti_data1.tm_min, 22);
    ++v;
    ++v;
    ++v;
    ++v;
    // time 5
    // TS_ASSERT(!isNaN(v->second));
    // last time
    std::map<DateAndTime, double>::reverse_iterator rv = vmap.rbegin();
    TS_ASSERT_EQUALS(rv->second, 9);
    const auto &ti_dataLast = rv->first.to_tm();
    TS_ASSERT_EQUALS(ti_dataLast.tm_hour, 14);
    TS_ASSERT_EQUALS(ti_dataLast.tm_min, 3);
    TS_ASSERT_DELTA(timeMean(p1.get()), 8.4904, 0.001);
  }

  //----------------------------------------------------------------------------
  void test_timeMean() {
    constexpr size_t logSize(11);
    auto log = std::make_unique<TimeSeriesProperty<double>>("MydoubleLog");
    std::vector<double> values(logSize);
    std::iota(values.begin(), values.end(), 1);
    DateAndTime firstTime("2007-11-30T16:17:00");
    std::vector<DateAndTime> times(logSize);
    std::generate(times.begin(), times.end(), [&firstTime] { return firstTime += 10.0; });
    TS_ASSERT_THROWS_NOTHING(log->addValues(times, values));
    TS_ASSERT_EQUALS(log->realSize(), logSize);
    TS_ASSERT_DELTA(timeMean(log.get()), 6.0, 1e-3);
  }

  void test_timeMean_one_Value() {
    auto log = std::make_unique<TimeSeriesProperty<double>>("MydoubleLog");
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:00", 56));
    TS_ASSERT_EQUALS(log->realSize(), 1);

    TS_ASSERT_DELTA(timeMean(log.get()), 56.0, 1e-3);
  }

  /// Tests to see if we can cope with duplicate log values that have the same
  /// time.
  void test_timeMean_duplicate_values_with_same_timestamp() {
    auto log = std::make_unique<TimeSeriesProperty<double>>("MydoubleLog");
    // Add the same value twice
    TS_ASSERT_THROWS_NOTHING(log->addValue("2012-07-19T20:00:00", 666));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2012-07-19T20:00:00", 666));
    TS_ASSERT_EQUALS(log->realSize(), 2);
    TS_ASSERT_DELTA(timeMean(log.get()), 666, 1e-3);
  }

  void test_isICPEventLogNewStyle_works() {
    auto oldlog = std::make_unique<TimeSeriesProperty<std::string>>("MyOldICPevent");
    TS_ASSERT_THROWS_NOTHING(oldlog->addValue("2012-07-19T20:00:00", "START"));
    TS_ASSERT_THROWS_NOTHING(oldlog->addValue("2012-07-19T20:00:01", "BEGIN"));
    TS_ASSERT_THROWS_NOTHING(oldlog->addValue("2012-07-19T20:00:02", "PAUSE"));

    const auto &oldLogm = oldlog->valueAsMultiMap();
    TS_ASSERT(!LogParser::isICPEventLogNewStyle(oldLogm));

    auto newlog = std::make_unique<TimeSeriesProperty<std::string>>("MyNewICPevent");
    TS_ASSERT_THROWS_NOTHING(newlog->addValue("2012-07-19T20:00:00", "START"));
    TS_ASSERT_THROWS_NOTHING(newlog->addValue("2012-07-19T20:00:01", "START_COLLECTION PERIOD 1"));
    TS_ASSERT_THROWS_NOTHING(newlog->addValue("2012-07-19T20:00:02", "PAUSE"));

    const auto &logm = newlog->valueAsMultiMap();
    TS_ASSERT(LogParser::isICPEventLogNewStyle(logm));

    newlog.reset(new TimeSeriesProperty<std::string>("MyNewICPevent1"));
    TS_ASSERT_THROWS_NOTHING(newlog->addValue("2012-07-19T20:00:00", "START"));
    TS_ASSERT_THROWS_NOTHING(newlog->addValue("2012-07-19T20:00:01", "STOP_COLLECTION PERIOD 1"));
    TS_ASSERT_THROWS_NOTHING(newlog->addValue("2012-07-19T20:00:02", "PAUSE"));

    const auto &newLogm = newlog->valueAsMultiMap();
    TS_ASSERT(LogParser::isICPEventLogNewStyle(newLogm));
  }

  void test_new_style_command_parsing() {
    auto log = std::make_unique<TimeSeriesProperty<std::string>>("MyICPevent");
    log->addValue("2013-10-16T19:04:47", "CHANGE_PERIOD 1");
    log->addValue("2013-10-16T19:04:48", "RESUME");
    log->addValue("2013-10-16T19:04:48", "START_COLLECTION PERIOD 1 GF 60015 RF 75039 GUAH 69.875610");
    log->addValue("2013-10-16T19:06:53", "STOP_COLLECTION PERIOD 1 GF 65024 RF 81303 GUAH 75.712013 DUR 125");
    log->addValue("2013-10-16T19:06:53", "PAUSE");
    log->addValue("2013-10-16T19:06:53", "CHANGE_PERIOD 2");
    log->addValue("2013-10-16T19:06:53", "RESUME");
    log->addValue("2013-10-16T19:06:53", "START_COLLECTION PERIOD 2 GF 65024 RF 81303 GUAH 75.712013");
    log->addValue("2013-10-16T19:08:58", "STOP_COLLECTION PERIOD 2 GF 70033 RF 87567 GUAH 81.547050 DUR 125");
    log->addValue("2013-10-16T19:08:58", "PAUSE");
    log->addValue("2013-10-16T19:08:58", "CHANGE_PERIOD 1");
    log->addValue("2013-10-16T19:08:59", "RESUME");
    log->addValue("2013-10-16T19:08:59", "START_COLLECTION PERIOD 1 GF 70033 RF 87567 GUAH 81.547050");
    log->addValue("2013-10-16T19:11:03", "STOP_COLLECTION PERIOD 1 GF 75005 RF 93784 GUAH 87.339035 DUR 124");
    log->addValue("2013-10-16T19:11:03", "PAUSE");
    log->addValue("2013-10-16T19:11:03", "CHANGE_PERIOD 2");
    log->addValue("2013-10-16T19:11:04", "RESUME");
    log->addValue("2013-10-16T19:11:04", "START_COLLECTION PERIOD 2 GF 75005 RF 93784 GUAH 87.339035");
    log->addValue("2013-10-16T19:13:09", "STOP_COLLECTION PERIOD 2 GF 80016 RF 100049 GUAH 93.174751 DUR 125");
    log->addValue("2013-10-16T19:13:09", "PAUSE");
    log->addValue("2013-10-16T19:13:09", "CHANGE_PERIOD 1");
    log->addValue("2013-10-16T19:13:09", "RESUME");

    const std::vector<std::pair<std::string, int>> checkPeriod{{"2013-10-16T19:04:47", 1},
                                                               {"2013-10-16T19:06:53", 2},
                                                               {"2013-10-16T19:08:58", 1},
                                                               {"2013-10-16T19:11:03", 2},
                                                               {"2013-10-16T19:13:09", 1}};

    const std::vector<std::pair<std::string, bool>> checkRunning{
        {"2013-10-16T19:04:48", true},  {"2013-10-16T19:06:53", false}, {"2013-10-16T19:06:53", true},
        {"2013-10-16T19:08:58", false}, {"2013-10-16T19:08:59", true},  {"2013-10-16T19:11:03", false},
        {"2013-10-16T19:11:04", true},  {"2013-10-16T19:13:09", false}};

    const LogParser logparser(log.get());

    auto prop = std::unique_ptr<Property>(logparser.createAllPeriodsLog());
    const auto *allPeriodsProp = dynamic_cast<const TimeSeriesProperty<int> *>(prop.get());
    TS_ASSERT(allPeriodsProp);

    TS_ASSERT_EQUALS(5, allPeriodsProp->size());
    auto logm = allPeriodsProp->valueAsMultiMap();
    size_t i = 0;
    for (auto it = logm.begin(); it != logm.end(); ++it) {
      TS_ASSERT_EQUALS(it->first.toISO8601String(), checkPeriod[i].first);
      TS_ASSERT_EQUALS(it->second, checkPeriod[i].second);
      ++i;
    }

    prop.reset(logparser.createRunningLog());
    const auto *runningProp = dynamic_cast<const TimeSeriesProperty<bool> *>(prop.get());
    TS_ASSERT(runningProp);

    TS_ASSERT_EQUALS(8, runningProp->size());
    auto logm1 = runningProp->valueAsMultiMap();
    i = 0;
    for (auto it = logm1.begin(); it != logm1.end(); ++it) {
      TS_ASSERT_EQUALS(it->first.toISO8601String(), checkRunning[i].first);
      TS_ASSERT_EQUALS(it->second, checkRunning[i].second);
      ++i;
    }
  }

  void test_str_repeat() {
    mkStrRepeat();
    const auto &prop = std::unique_ptr<Property>(LogParser::createLogProperty(log_str_repeat.path(), "log"));
    const auto *log = dynamic_cast<const TimeSeriesProperty<std::string> *>(prop.get());
    TS_ASSERT(log);
    const auto &logm = log->valueAsMultiMap();
    auto it = logm.begin();
    TS_ASSERT_EQUALS(it->first.toISO8601String(), "2000-09-05T12:22:34");
    TS_ASSERT_EQUALS(it->second, "   First line");
    ++it;
    TS_ASSERT_EQUALS(it->first.toISO8601String(), "2000-09-05T12:22:34");
    TS_ASSERT_EQUALS(it->second, "   Second line");
    ++it;
    TS_ASSERT_EQUALS(it->first.toISO8601String(), "2000-09-05T12:23:33");
    TS_ASSERT_EQUALS(it->second, "   First line");
    ++it;
    TS_ASSERT_EQUALS(it->first.toISO8601String(), "2000-09-05T12:23:33");
    TS_ASSERT_EQUALS(it->second, "   Second line");
    ++it;
    TS_ASSERT_EQUALS(it->first.toISO8601String(), "2000-09-05T12:23:33");
    TS_ASSERT_EQUALS(it->second, "   Third line");
    ++it;
    TS_ASSERT_EQUALS(it->first.toISO8601String(), "2000-09-05T12:23:33");
    TS_ASSERT_EQUALS(it->second, "   Fourth line");
    ++it;
  }

  void test_num_repeat() {
    mkNumRepeat();
    const auto &prop = std::unique_ptr<Property>(LogParser::createLogProperty(log_str_repeat.path(), "log"));
    const auto *log = dynamic_cast<const TimeSeriesProperty<double> *>(prop.get());
    TS_ASSERT(log);
    const auto &logm = log->valueAsMultiMap();
    auto it = logm.begin();
    TS_ASSERT_EQUALS(it->first.toISO8601String(), "2000-09-05T12:22:34");
    TS_ASSERT_EQUALS(it->second, 1);
    ++it;
    TS_ASSERT_EQUALS(it->first.toISO8601String(), "2000-09-05T12:22:34");
    TS_ASSERT_EQUALS(it->second, 2);
    ++it;
    TS_ASSERT_EQUALS(it->first.toISO8601String(), "2000-09-05T12:23:33");
    TS_ASSERT_EQUALS(it->second, 3);
    ++it;
    TS_ASSERT_EQUALS(it->first.toISO8601String(), "2000-09-05T12:23:33");
    TS_ASSERT_EQUALS(it->second, 4);
    ++it;
    TS_ASSERT_EQUALS(it->first.toISO8601String(), "2000-09-05T12:23:33");
    TS_ASSERT_EQUALS(it->second, 5);
    ++it;
    TS_ASSERT_EQUALS(it->first.toISO8601String(), "2000-09-05T12:23:33");
    TS_ASSERT_EQUALS(it->second, 6);
    ++it;
  }

  void test_str_continuation() {
    mkStrContinuations();
    const auto &prop = std::unique_ptr<Property>(LogParser::createLogProperty(log_str_continuations.path(), "log"));
    const auto *log = dynamic_cast<const TimeSeriesProperty<std::string> *>(prop.get());
    TS_ASSERT(log);
    const auto &logm = log->valueAsMultiMap();
    auto it = logm.begin();
    TS_ASSERT_EQUALS(it->first.toISO8601String(), "2000-09-05T12:22:31");
    TS_ASSERT_EQUALS(it->second, "   First line Second line");
    ++it;
    TS_ASSERT_EQUALS(it->first.toISO8601String(), "2000-09-05T12:22:34");
    TS_ASSERT_EQUALS(it->second, "   First line");
    ++it;
    TS_ASSERT_EQUALS(it->first.toISO8601String(), "2000-09-05T12:22:34");
    TS_ASSERT_EQUALS(it->second, "   Second line Third line");
    ++it;
  }

  /// If a run is aborted and then restarted, the "running" log should be set to
  /// false at all times during the aborted run.
  void test_abort_runningLogAlwaysFalseBeforeRestart() {
    auto log = std::make_unique<TimeSeriesProperty<std::string>>("MyICPevent");

    // (This is a cut-down version of EMU66122)
    const DateAndTime timeZero{"2016-10-01T10:01:44"};
    const std::vector<DateAndTime> times{timeZero,        timeZero + 3.0,  timeZero + 3.0,   timeZero + 3.0,
                                         timeZero + 51.0, timeZero + 51.0, timeZero + 57.0,  timeZero + 60.0,
                                         timeZero + 60.0, timeZero + 60.0, timeZero + 111.0, timeZero + 111.0};
    const std::vector<std::string> values{"CHANGE_PERIOD 1",
                                          "CHANGE_PERIOD 1",
                                          "START_COLLECTION PERIOD 1 GF 0 RF 0 GUAH 0.000000",
                                          "BEGIN",
                                          "STOP_COLLECTION PERIOD 1 GF 1931 RF 1933 GUAH 0.000000 DUR 48",
                                          "ABORT",
                                          "CHANGE_PERIOD 1",
                                          "CHANGE_PERIOD 1",
                                          "START_COLLECTION PERIOD 1 GF 0 RF 0 GUAH 0.000000",
                                          "BEGIN",
                                          "STOP_COLLECTION PERIOD 1 GF 2062 RF 2064 GUAH 0.000000 DUR 51",
                                          "END"};
    log->addValues(times, values);

    const std::multimap<DateAndTime, bool> expectedRunning{{timeZero + 3.0, false},    // start - run later aborted
                                                           {timeZero + 51.0, false},   // stop
                                                           {timeZero + 51.0, false},   // abort
                                                           {timeZero + 60.0, true},    // start
                                                           {timeZero + 111.0, false}}; // stop

    const LogParser logparser(log.get());

    const auto &prop = std::unique_ptr<Property>(logparser.createRunningLog());
    const auto *runningProp = dynamic_cast<const TimeSeriesProperty<bool> *>(prop.get());
    TS_ASSERT(runningProp);
    TS_ASSERT_EQUALS(expectedRunning.size(), runningProp->size());
    const auto &runningMap = runningProp->valueAsMultiMap();
    TS_ASSERT_EQUALS(expectedRunning, runningMap);
  }

  /// If a run is aborted and then restarted, the "running" log should be set to
  /// false at all times during the aborted run.
  void test_abort_runningLogAlwaysFalseBeforeRestart_oldStyleCommands() {
    auto log = std::make_unique<TimeSeriesProperty<std::string>>("MyICPevent");

    // (This is a cut-down version of EMU66122, changed to "old style" commands)
    const DateAndTime timeZero{"2016-10-01T10:01:44"};
    const std::vector<DateAndTime> times{timeZero,        timeZero + 3.0,  timeZero + 3.0,  timeZero + 51.0,
                                         timeZero + 57.0, timeZero + 60.0, timeZero + 60.0, timeZero + 111.0};
    const std::vector<std::string> values{"CHANGE_PERIOD 1", "CHANGE_PERIOD 1", "BEGIN", "ABORT",
                                          "CHANGE_PERIOD 1", "CHANGE_PERIOD 1", "BEGIN", "END"};
    log->addValues(times, values);

    const std::multimap<DateAndTime, bool> expectedRunning{{timeZero + 3.0, false},    // begin - run later aborted
                                                           {timeZero + 51.0, false},   // abort
                                                           {timeZero + 60.0, true},    // begin
                                                           {timeZero + 111.0, false}}; // end

    const LogParser logparser(log.get());

    const auto &prop = std::unique_ptr<Property>(logparser.createRunningLog());
    const auto *runningProp = dynamic_cast<const TimeSeriesProperty<bool> *>(prop.get());
    TS_ASSERT(runningProp);
    TS_ASSERT_EQUALS(expectedRunning.size(), runningProp->size());
    const auto &runningMap = runningProp->valueAsMultiMap();
    TS_ASSERT_EQUALS(expectedRunning, runningMap);
  }

private:
  /// Helper method to run common test code for checking period logs.
  void doTestCurrentPeriodLog(const int &expected_period) {
    const auto &log = std::make_unique<TimeSeriesProperty<std::string>>("ICPLog");
    const LogParser logparser(log.get());
    const auto prop = std::unique_ptr<Property>(logparser.createCurrentPeriodLog(expected_period));
    const auto *prop_with_value = dynamic_cast<PropertyWithValue<int> *>(prop.get());

    TS_ASSERT(prop_with_value != nullptr);
    int value = boost::lexical_cast<int>(prop_with_value->value());
    TS_ASSERT_EQUALS(expected_period, value);
  }

  void mkICP() {
    std::ofstream f(icp_file.path().c_str());
    f << "2000-09-05T12:22:28   START_SE_WAIT\n";
    f << "2000-09-05T12:22:33   BEGIN\n";
    f << "2000-09-05T12:22:41   PAUSE\n";
    f << "2000-09-05T12:22:55   CHANGE PERIOD 2\n";
    f << "2000-09-05T12:22:58   RESUME\n";
    f << "2000-09-05T12:23:04   PAUSE\n";
    f << "2000-09-05T12:23:08   CHANGE PERIOD 1\n";
    f << "2000-09-05T12:23:10   RESUME\n";
    f << "2000-09-05T12:23:18   START_SE_WAIT\n";
    f << "2000-09-05T12:23:22   CHANGE PERIOD 2\n";
    f << "2000-09-05T12:23:27   RESUME\n";
    f << "2000-09-05T12:23:34   ABORT\n";
    f << "2000-09-05T12:23:37   CHANGE PERIOD 1\n";
    f << "2000-09-05T12:23:42   END_SE_WAIT\n";
    f << "2000-09-05T14:03:54   END\n";
    f.close();
  }

  void mkICPVariant() {
    std::ofstream f(icp_file.path().c_str());
    f << "2000-09-05T12:22:28   START_SE_WAIT\n";
    f << "2000-09-05T12:22:33   BEGIN\n";
    f << "2000-09-05T12:22:41   PAUSE\n";
    f << "2000-09-05T12:22:55   CHANGE_PERIOD 2\n";
    f << "2000-09-05T12:22:58   RESUME\n";
    f << "2000-09-05T12:23:04   PAUSE\n";
    f << "2000-09-05T12:23:08   CHANGE_PERIOD 1\n";
    f << "2000-09-05T12:23:10   RESUME\n";
    f << "2000-09-05T12:23:18   START_SE_WAIT\n";
    f << "2000-09-05T12:23:22   CHANGE_PERIOD 2\n";
    f << "2000-09-05T12:23:27   RESUME\n";
    f << "2000-09-05T12:23:34   ABORT\n";
    f << "2000-09-05T12:23:37   CHANGE_PERIOD 1\n";
    f << "2000-09-05T12:23:42   END_SE_WAIT\n";
    f << "2000-09-05T14:03:54   END\n";
    f.close();
  }

  void mkGood() {
    std::ofstream f(log_num_good.path().c_str());
    f << "2000-09-05T12:22:31   " << 1 << '\n';
    f << "2000-09-05T12:22:37   " << 2 << '\n';
    f << "2000-09-05T12:22:38   " << 3 << '\n';
    f << "2000-09-05T12:22:39   " << 4 << '\n';
    f << "2000-09-05T12:22:42   " << 5 << '\n';
    f << "2000-09-05T12:22:47   " << 6 << '\n';
    f << "2000-09-05T12:22:56   " << 7 << '\n';
    f << "2000-09-05T12:23:00   " << 8 << '\n';
    f << "2000-09-05T14:03:56   " << 9 << '\n';
    f.close();
  }

  void mkLate() {
    std::ofstream f(log_num_late.path().c_str());
    f << "2000-09-05T12:22:37   " << 2 << '\n';
    f << "2000-09-05T12:22:38   " << 3 << '\n';
    f << "2000-09-05T12:22:39   " << 4 << '\n';
    f << "2000-09-05T12:22:42   " << 5 << '\n';
    f << "2000-09-05T12:22:47   " << 6 << '\n';
    f << "2000-09-05T12:22:56   " << 7 << '\n';
    f << "2000-09-05T12:23:00   " << 8 << '\n';
    f << "2000-09-05T14:03:56   " << 9 << '\n';
    f.close();
  }

  void mkEarly() {
    std::ofstream f(log_num_early.path().c_str());
    f << "2000-09-05T12:22:31   " << 1 << '\n';
    f << "2000-09-05T12:22:37   " << 2 << '\n';
    f << "2000-09-05T12:22:38   " << 3 << '\n';
    f << "2000-09-05T12:22:39   " << 4 << '\n';
    f << "2000-09-05T12:22:42   " << 5 << '\n';
    f << "2000-09-05T12:22:47   " << 6 << '\n';
    f << "2000-09-05T12:22:56   " << 7 << '\n';
    f << "2000-09-05T12:23:00   " << 8 << '\n';
    f.close();
  }

  void mkSingle() {
    std::ofstream f(log_num_single.path().c_str());
    f << "2000-09-05T12:22:51   " << 4 << '\n';
    f.close();
  }

  void mkStr() {
    std::ofstream f(log_str.path().c_str());
    f << "2000-09-05T12:22:31   line " << 1 << '\n';
    f << "2000-09-05T12:22:37   line " << 2 << '\n';
    f << "2000-09-05T12:22:38   line " << 3 << '\n';
    f << "2000-09-05T12:22:39   line " << 4 << '\n';
    f << "2000-09-05T12:22:42   line " << 5 << '\n';
    f << "2000-09-05T12:22:47   line " << 6 << '\n';
    f << "2000-09-05T12:22:56   line " << 7 << '\n';
    f << "2000-09-05T12:23:00   line " << 8 << '\n';
    f << "2000-09-05T14:03:56   line " << 9 << '\n';
    f.close();
  }

  void mkStrContinuations() {
    std::ofstream f(log_str_continuations.path().c_str());
    f << "2000-09-05T12:22:31   First line\n";
    f << "Second line\n";
    f << "2000-09-05T12:22:34   First line\n";
    f << "2000-09-05T12:22:34   Second line\n";
    f << "Third line\n";
    f.close();
  }

  void mkStrRepeat() {
    std::ofstream f(log_str_repeat.path().c_str());
    f << "2000-09-05T12:22:34   First line\n";
    f << "2000-09-05T12:22:34   Second line\n";
    f << "2000-09-05T12:23:33   First line\n";
    f << "2000-09-05T12:23:33   Second line\n";
    f << "2000-09-05T12:23:33   Third line\n";
    f << "2000-09-05T12:23:33   Fourth line\n";
    f.close();
  }

  void mkNumRepeat() {
    std::ofstream f(log_str_repeat.path().c_str());
    f << "2000-09-05T12:22:34   1\n";
    f << "2000-09-05T12:22:34   2\n";
    f << "2000-09-05T12:23:33   3\n";
    f << "2000-09-05T12:23:33   4\n";
    f << "2000-09-05T12:23:33   5\n";
    f << "2000-09-05T12:23:33   6\n";
    f.close();
  }

  TmpFile log_num_good;          // run time interval is within first - last times of the log
  TmpFile log_num_late;          // first time is later than run start
  TmpFile log_num_early;         // last time is earlier than run ends
  TmpFile log_num_single;        // single value
  TmpFile log_str;               // file of strings
  TmpFile icp_file;              // icpevent file
  TmpFile log_str_repeat;        // string log with repeating lines
  TmpFile log_num_repeat;        // num log with repeating lines
  TmpFile log_str_continuations; // string log with continuation lines
};
