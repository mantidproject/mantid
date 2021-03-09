// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Run.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/NexusTestHelper.h"

#include <cxxtest/TestSuite.h>
#include <json/value.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// Helper class
namespace {
class ConcreteProperty : public Property {
public:
  ConcreteProperty() : Property("Test", typeid(int)) {}
  ConcreteProperty *clone() const override {
    return new ConcreteProperty(*this);
  }
  bool isDefault() const override { return true; }
  std::string getDefault() const override {
    return "getDefault() is not implemented in this class";
  }
  std::string value() const override { return "Nothing"; }
  Json::Value valueAsJson() const override { return Json::Value(); }
  std::string setValue(const std::string &) override { return ""; }
  std::string setValueFromJson(const Json::Value &) override { return ""; }
  std::string setValueFromProperty(const Property &) override { return ""; }
  std::string setDataItem(const std::shared_ptr<DataItem> &) override {
    return "";
  }
  Property &operator+=(Property const *) override { return *this; }
};

void addTestTimeSeries(Run &run, const std::string &name) {
  auto timeSeries = new TimeSeriesProperty<double>(name);
  timeSeries->addValue("2012-07-19T16:17:00", 2);
  timeSeries->addValue("2012-07-19T16:17:10", 3);
  timeSeries->addValue("2012-07-19T16:17:20", 4);
  timeSeries->addValue("2012-07-19T16:17:30", 5);
  timeSeries->addValue("2012-07-19T16:17:40", 6);
  timeSeries->addValue("2012-07-19T16:17:50", 20);
  timeSeries->addValue("2012-07-19T16:18:00", 21);
  timeSeries->addValue("2012-07-19T16:18:10", 22);
  timeSeries->addValue("2012-07-19T16:19:20", 23);
  timeSeries->addValue("2012-07-19T16:19:20", 24);
  run.addProperty(timeSeries);
}
} // namespace

class RunTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RunTest *createSuite() { return new RunTest(); }
  static void destroySuite(RunTest *suite) { delete suite; }

  RunTest() : m_test_energy_bins(5) {
    m_test_energy_bins[0] = -1.1;
    m_test_energy_bins[1] = -0.2;
    m_test_energy_bins[2] = 0.7;
    m_test_energy_bins[3] = 1.6;
    m_test_energy_bins[4] = 3.2;
  }

  void testAddGetData() {
    Run runInfo;

    Property *p = new ConcreteProperty();
    TS_ASSERT_THROWS_NOTHING(runInfo.addProperty(p));

    Property *pp = nullptr;
    TS_ASSERT_THROWS_NOTHING(pp = runInfo.getProperty("Test"));
    TS_ASSERT_EQUALS(p, pp);
    TS_ASSERT(!pp->name().compare("Test"));
    TS_ASSERT(dynamic_cast<ConcreteProperty *>(pp));
    TS_ASSERT_THROWS(pp = runInfo.getProperty("NotThere"),
                     const Exception::NotFoundError &);

    std::vector<Property *> props = runInfo.getProperties();
    TS_ASSERT(!props.empty());
    TS_ASSERT_EQUALS(props.size(), 1);
    TS_ASSERT(!props[0]->name().compare("Test"));
    TS_ASSERT(dynamic_cast<ConcreteProperty *>(props[0]));
  }

  void testRemoveLogData() {
    Run runInfo;

    Property *p = new ConcreteProperty();
    TS_ASSERT_THROWS_NOTHING(runInfo.addProperty(p));
    TS_ASSERT_THROWS_NOTHING(runInfo.removeProperty("Test"));
    TS_ASSERT_EQUALS(runInfo.getProperties().size(), 0);
  }

  void testGetSetProtonCharge() {
    Run runInfo;
    TS_ASSERT_EQUALS(runInfo.getProtonCharge(), 0.0);
    TS_ASSERT_THROWS_NOTHING(runInfo.setProtonCharge(10.0));
    TS_ASSERT_EQUALS(runInfo.getProtonCharge(), 10.0);
  }

  void testCopyAndAssignment() {
    Run runInfo;
    runInfo.setProtonCharge(10.0);
    Property *p = new ConcreteProperty();
    TS_ASSERT_THROWS_NOTHING(runInfo.addProperty(p));
    TS_ASSERT_EQUALS(runInfo.getProperties().size(), 2);

    // Copy constructor
    Run runInfo_2(runInfo);
    TS_ASSERT_EQUALS(runInfo_2.getProperties().size(), 2);
    TS_ASSERT_DELTA(runInfo_2.getProtonCharge(), 10.0, 1e-8);
    TS_ASSERT_EQUALS(runInfo_2.getLogData("Test")->value(), "Nothing");

    // Now assignment
    runInfo.setProtonCharge(15.0);
    runInfo.removeProperty("Test");
    runInfo_2 = runInfo;
    TS_ASSERT_EQUALS(runInfo_2.getProperties().size(), 1);
    TS_ASSERT_DELTA(runInfo_2.getProtonCharge(), 15.0, 1e-8);
  }

  void testMemory() {
    Run runInfo;
    TS_ASSERT_EQUALS(runInfo.getMemorySize(), sizeof(Goniometer));

    Property *p = new ConcreteProperty();
    runInfo.addProperty(p);
    size_t classSize =
        sizeof(ConcreteProperty) + sizeof(void *) + sizeof(Goniometer);
    TS_ASSERT_EQUALS(classSize, runInfo.getMemorySize());
  }

  void test_GetTimeSeriesProperty_Returns_TSP_When_Log_Exists() {
    Run runInfo;
    const std::string &name = "double_time_series";
    const double value = 10.9;
    addTimeSeriesEntry(runInfo, name, value);

    TimeSeriesProperty<double> *tsp(nullptr);
    TS_ASSERT_THROWS_NOTHING(tsp = runInfo.getTimeSeriesProperty<double>(name));
    TS_ASSERT_DELTA(tsp->firstValue(), value, 1e-12);
  }

  void test_GetTimeSeriesProperty_Throws_When_Log_Does_Not_Exist() {
    Run runInfo;
    TS_ASSERT_THROWS(runInfo.getTimeSeriesProperty<double>("not_a_log"),
                     const Exception::NotFoundError &);
  }

  void
  test_GetTimeSeriesProperty_Throws_When_Log_Exists_But_Is_Not_Correct_Type() {
    Run runInfo;
    const std::string &name = "double_prop";
    runInfo.addProperty(name, 5.6); // Standard double property

    TS_ASSERT_THROWS(runInfo.getTimeSeriesProperty<double>(name),
                     const std::invalid_argument &);
  }

  void test_GetPropertyAsType_Throws_When_Property_Does_Not_Exist() {
    Run runInfo;
    TS_ASSERT_THROWS(runInfo.getPropertyValueAsType<double>("not_a_log"),
                     const Exception::NotFoundError &);
  }

  void test_GetPropertyAsType_Returns_Expected_Value_When_Type_Is_Correct() {
    Run runInfo;
    const std::string &name = "double_prop";
    const double value = 5.6;
    runInfo.addProperty(name, value); // Standard double property

    double retrieved(0.0);
    TS_ASSERT_THROWS_NOTHING(retrieved =
                                 runInfo.getPropertyValueAsType<double>(name));
    TS_ASSERT_DELTA(retrieved, value, 1e-12);
  }

  void test_GetPropertyAsType_Throws_When_Requested_Type_Does_Not_Match() {
    Run runInfo;
    runInfo.addProperty("double_prop", 6.7); // Standard double property

    TS_ASSERT_THROWS(runInfo.getPropertyValueAsType<int>("double_prop"),
                     const std::invalid_argument &);
  }

  void
  test_GetPropertyAsSingleValue_Throws_If_Type_Is_Not_Numeric_Or_TimeSeries_Numeric() {
    Run runInfo;
    const std::string name = "string_prop";
    runInfo.addProperty<std::string>(name, "string"); // Adds a string property

    TS_ASSERT_THROWS(runInfo.getPropertyAsSingleValue(name),
                     const std::invalid_argument &);
  }

  void test_GetPropertyAsSingleValue_Does_Not_Throw_If_Type_Is_Int() {
    Run runInfo;
    const std::string name = "int_prop";
    runInfo.addProperty(name, 1); // Adds an int property

    TS_ASSERT_THROWS_NOTHING(runInfo.getPropertyAsSingleValue(name));
  }

  void
  test_GetPropertyAsSingleValue_Returns_Simple_Mean_By_Default_For_Time_Series() {
    Run runInfo;
    const std::string name = "series";
    addTestTimeSeries(runInfo, name);

    const double expectedValue(13.0);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name), expectedValue,
                    1e-12);
  }

  void
  test_GetPropertyAsSingleValue_Returns_Correct_SingleValue_For_Each_StatisticType() {
    Run runInfo;
    const std::string name = "series";
    addTestTimeSeries(runInfo, name);

    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::Mean), 13.0,
                    1e-12);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::Minimum), 2.0,
                    1e-12);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::Maximum), 24.0,
                    1e-12);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::FirstValue),
                    2.0, 1e-12);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::LastValue),
                    24.0, 1e-12);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::Median), 13.0,
                    1e-12);
  }

  void
  test_GetPropertyAsSingleValue_Returns_Expected_Single_Value_On_Successive_Calls_With_Different_Stat_Types() {
    Run run;
    const std::string name = "series";
    addTestTimeSeries(run, name);

    TS_ASSERT_EQUALS(run.getPropertyAsSingleValue(name, Math::Mean), 13.0);
    TS_ASSERT_EQUALS(run.getPropertyAsSingleValue(name, Math::Mean), 13.0);
    TS_ASSERT_EQUALS(run.getPropertyAsSingleValue(name, Math::Minimum), 2.0);
    TS_ASSERT_EQUALS(run.getPropertyAsSingleValue(name, Math::Minimum), 2.0);
  }

  void
  test_GetPropertyAsSingleValue_Returns_Correct_Value_On_Second_Call_When_Log_Has_Been_Replaced() {
    Run runInfo;
    const std::string name = "double";
    double value(5.1);
    runInfo.addProperty(name, value);

    TS_ASSERT_EQUALS(runInfo.getPropertyAsSingleValue(name), value);

    // Replace the log with a different value
    value = 10.3;
    runInfo.addProperty(name, value, /*overwrite*/ true);

    TS_ASSERT_EQUALS(runInfo.getPropertyAsSingleValue(name), value);
  }

  void
  test_storeHistogramBinBoundaries_Throws_If_Fewer_Than_Two_Values_Are_Given() {
    Run runInfo;

    std::vector<double> bins;
    TS_ASSERT_THROWS(runInfo.storeHistogramBinBoundaries(bins),
                     const std::invalid_argument &);
    bins.emplace_back(0.5);
    TS_ASSERT_THROWS(runInfo.storeHistogramBinBoundaries(bins),
                     const std::invalid_argument &);
    bins.emplace_back(1.5);
    TS_ASSERT_THROWS_NOTHING(runInfo.storeHistogramBinBoundaries(bins));
  }

  void
  test_storeHistogramBinBoundaries_Throws_If_First_Value_Is_Greater_Or_Equal_To_Last_Value() {
    Run runInfo;
    std::vector<double> bins(2, 0.0);

    TS_ASSERT_THROWS(runInfo.storeHistogramBinBoundaries(bins),
                     const std::out_of_range &);

    bins[0] = -1.5;
    bins[1] = -1.5;
    TS_ASSERT_THROWS(runInfo.storeHistogramBinBoundaries(bins),
                     const std::out_of_range &);

    bins[0] = 2.1;
    bins[1] = 2.1;
    TS_ASSERT_THROWS(runInfo.storeHistogramBinBoundaries(bins),
                     const std::out_of_range &);

    bins[0] = -1.5;
    bins[1] = -1.6;
    TS_ASSERT_THROWS(runInfo.storeHistogramBinBoundaries(bins),
                     const std::out_of_range &);

    bins[0] = 2.1;
    bins[1] = 1.9;
    TS_ASSERT_THROWS(runInfo.storeHistogramBinBoundaries(bins),
                     const std::out_of_range &);
  }

  void test_storeHistogramBinBoundaries_Succeeds_With_Valid_Bins() {
    Run runInfo;

    TS_ASSERT_THROWS_NOTHING(
        runInfo.storeHistogramBinBoundaries(m_test_energy_bins));
    TS_ASSERT_THROWS_NOTHING(
        runInfo.histogramBinBoundaries(m_test_energy_bins[1] + 0.1));
  }

  void test_histogramBinBoundaries_Throws_RuntimeError_For_New_Run() {
    Run runInfo;

    TS_ASSERT_THROWS(runInfo.histogramBinBoundaries(1.5),
                     const std::runtime_error &);
  }

  void
  test_histogramBinBoundaries_Throws_RuntimeError_When_Value_Is_Outside_Boundaries_Range() {
    using namespace Mantid::Kernel;
    Run runInfo;
    runInfo.storeHistogramBinBoundaries(m_test_energy_bins);

    TS_ASSERT_THROWS(
        runInfo.histogramBinBoundaries(m_test_energy_bins.front() - 1.3),
        const std::out_of_range &);
    TS_ASSERT_THROWS(
        runInfo.histogramBinBoundaries(m_test_energy_bins.back() + 1.3),
        const std::out_of_range &);
  }

  void
  test_histogramBinBoundaries_Returns_Closest_Lower_And_Upper_Boundary_For_Valid_Bin_Value_Away_From_Any_Edge() {
    using namespace Mantid::Kernel;
    Run runInfo;
    runInfo.storeHistogramBinBoundaries(m_test_energy_bins);

    std::pair<double, double> edges;
    TS_ASSERT_THROWS_NOTHING(edges = runInfo.histogramBinBoundaries(1.2));

    TS_ASSERT_DELTA(edges.first, 0.7, 1e-12);
    TS_ASSERT_DELTA(edges.second, 1.6, 1e-12);
  }

  void
  test_histogramBinBoundaries_Returns_The_Value_And_Next_Boundary_Along_If_Given_Value_Equals_A_Bin_Edge_Away_From_Ends() {
    using namespace Mantid::Kernel;
    Run runInfo;
    runInfo.storeHistogramBinBoundaries(m_test_energy_bins);

    std::pair<double, double> edges;
    TS_ASSERT_THROWS_NOTHING(edges = runInfo.histogramBinBoundaries(-0.2));
    TS_ASSERT_DELTA(edges.first, -0.2, 1e-12);
    TS_ASSERT_DELTA(edges.second, 0.7, 1e-12);
  }

  void
  test_histogramBinBoundaries_Returns_The_Value_And_Next_Boundary_Along_If_Given_Value_Equals_A_The_First_Bin_Edge() {
    using namespace Mantid::Kernel;
    Run runInfo;
    runInfo.storeHistogramBinBoundaries(m_test_energy_bins);

    std::pair<double, double> edges;
    TS_ASSERT_THROWS_NOTHING(
        edges = runInfo.histogramBinBoundaries(m_test_energy_bins.front()));
    TS_ASSERT_DELTA(edges.first, -1.1, 1e-12);
    TS_ASSERT_DELTA(edges.second, -0.2, 1e-12);
  }

  void
  test_histogramBinBoundaries_Returns_The_Value_And_Previous_Boundary_If_Given_Value_Equals_The_Last_Bin_Edge() {
    using namespace Mantid::Kernel;
    Run runInfo;
    runInfo.storeHistogramBinBoundaries(m_test_energy_bins);

    std::pair<double, double> edges;
    TS_ASSERT_THROWS_NOTHING(
        edges = runInfo.histogramBinBoundaries(m_test_energy_bins.back()));
    TS_ASSERT_DELTA(edges.first, 1.6, 1e-12);
    TS_ASSERT_DELTA(edges.second, 3.2, 1e-12);
  }

  void test_getBinBoundaries() {
    using namespace Mantid::Kernel;
    Run runInfo;
    runInfo.storeHistogramBinBoundaries(m_test_energy_bins);

    std::vector<double> bounds;
    TS_ASSERT_THROWS_NOTHING(bounds = runInfo.getBinBoundaries());
    for (size_t i = 0; i < m_test_energy_bins.size(); ++i) {
      TS_ASSERT_DELTA(bounds.at(i), m_test_energy_bins.at(i), 1e-12);
    }
  }

  void test_getGoniometer() {
    Run runInfo;
    TS_ASSERT_THROWS_NOTHING(runInfo.getGoniometer());
    // No axes by default
    TS_ASSERT_EQUALS(runInfo.getGoniometer().getNumberAxes(), 0);
    // Now does copy work?
    Goniometer gm;
    gm.makeUniversalGoniometer();
    runInfo.setGoniometer(gm, false);
    TS_ASSERT_EQUALS(runInfo.getGoniometer().getNumberAxes(), 3);
    Run runCopy(runInfo);
    TS_ASSERT_EQUALS(runCopy.getGoniometer().getNumberAxes(), 3);
    runCopy = runInfo;
    TS_ASSERT_EQUALS(runCopy.getGoniometer().getNumberAxes(), 3);
  }

  void test_multiple_goniometers() {
    Run runInfo;
    TS_ASSERT_EQUALS(runInfo.getNumGoniometers(), 1);

    DblMatrix rotation(3, 3, true);
    rotation[0][0] = cos(0.5);
    rotation[0][2] = sin(0.5);
    rotation[2][0] = -sin(0.5);
    rotation[2][2] = cos(0.5);
    Goniometer goniometer(rotation);

    auto index = runInfo.addGoniometer(goniometer);

    TS_ASSERT_EQUALS(runInfo.getNumGoniometers(), 2);
    TS_ASSERT_EQUALS(index, 1);

    TS_ASSERT_EQUALS(runInfo.getGoniometer(0), Goniometer());
    TS_ASSERT_EQUALS(runInfo.getGoniometer(1), goniometer);
    TS_ASSERT_EQUALS(runInfo.getGoniometerMatrix(0), DblMatrix(3, 3, true));
    TS_ASSERT_EQUALS(runInfo.getGoniometerMatrix(1), rotation);

    std::vector<DblMatrix> matrices = runInfo.getGoniometerMatrices();
    TS_ASSERT_EQUALS(matrices.size(), 2);
    TS_ASSERT_EQUALS(matrices[0], DblMatrix(3, 3, true));
    TS_ASSERT_EQUALS(matrices[1], rotation);

    Run runInfo2;
    TS_ASSERT_DIFFERS(runInfo, runInfo2);
    runInfo2.addGoniometer(Goniometer());
    TS_ASSERT_DIFFERS(runInfo, runInfo2);
    runInfo2.clearGoniometers();
    runInfo2.addGoniometer(Goniometer());
    runInfo2.addGoniometer(goniometer);
    TS_ASSERT_EQUALS(runInfo, runInfo2);

    runInfo.clearGoniometers();
    TS_ASSERT_EQUALS(runInfo.getNumGoniometers(), 0);

    TS_ASSERT_THROWS(runInfo.getGoniometer(0), const std::out_of_range &);
    TS_ASSERT_THROWS(runInfo.getGoniometerMatrix(0), const std::out_of_range &);
  }

  void addTimeSeriesEntry(Run &runInfo, const std::string &name, double val) {
    TimeSeriesProperty<double> *tsp;
    tsp = new TimeSeriesProperty<double>(name);
    tsp->addValue("2011-05-24T00:00:00", val);
    runInfo.addProperty(tsp);
  }

  void test_clear() {
    // Set up a Run object with 3 properties in it (1 time series, 2 single
    // value)
    Run runInfo;
    const std::string stringProp("aStringProp");
    const std::string stringVal("testing");
    runInfo.addProperty(stringProp, stringVal);
    const std::string intProp("anIntProp");
    runInfo.addProperty(intProp, 99);
    const std::string tspProp("tsp");
    addTestTimeSeries(runInfo, "tsp");

    // Check it's set up right
    TS_ASSERT_EQUALS(runInfo.getProperties().size(), 3);
    auto tsp = runInfo.getTimeSeriesProperty<double>(tspProp);
    TS_ASSERT_EQUALS(tsp->realSize(), 10)

    // Do the clearing work
    TS_ASSERT_THROWS_NOTHING(runInfo.clearTimeSeriesLogs());

    // Check the time-series property is empty, but not the others
    TS_ASSERT_EQUALS(runInfo.getProperties().size(), 3);
    TS_ASSERT_EQUALS(tsp->realSize(), 0)
    TS_ASSERT_EQUALS(runInfo.getPropertyValueAsType<std::string>(stringProp),
                     stringVal);
    TS_ASSERT_EQUALS(runInfo.getPropertyValueAsType<int>(intProp), 99);
  }

  /** Setting up a goniometer and the angles to feed it
   * using sample logs, then getting the right rotation matrix out.
   */
  void test_setGoniometerWhenLogsDoNotExistsThrows() {
    Run runInfo;
    Goniometer gm;
    gm.makeUniversalGoniometer();

    TS_ASSERT_THROWS(runInfo.setGoniometer(gm, true),
                     const std::runtime_error &);
  }

  /** Setting up a goniometer and the angles to feed it
   * using sample logs, then getting the right rotation matrix out.
   */
  void test_setGoniometer_Not_Using_Logs_Preserves_Input() {
    Run runInfo;
    DblMatrix rotation(3, 3, true);
    Goniometer gm(rotation);

    TS_ASSERT_EQUALS(runInfo.getGoniometer().getNumberAxes(), 0);
    TS_ASSERT_EQUALS(runInfo.getGoniometer().getR(), rotation);
  }

  /** Setting up a goniometer and the angles to feed it
   * using sample logs, then getting the right rotation matrix out.
   */
  void test_getGoniometerMatrix() {
    Run runInfo;
    addTimeSeriesEntry(runInfo, "phi", 90.0);
    addTimeSeriesEntry(runInfo, "chi", 90.0);
    addTimeSeriesEntry(runInfo, "omega", 90.0);
    Goniometer gm;
    gm.makeUniversalGoniometer();
    runInfo.setGoniometer(gm, true);
    DblMatrix r = runInfo.getGoniometerMatrix();
    V3D rot = r * V3D(-1, 0, 0);
    TS_ASSERT_EQUALS(rot, V3D(1, 0, 0));
    rot = r * V3D(0, 0, 1);
    TS_ASSERT_EQUALS(rot, V3D(0, 1, 0));
  }

  void test_getGoniometerMatrix2() {
    Run runInfo;
    addTimeSeriesEntry(runInfo, "phi", 45.0);
    addTimeSeriesEntry(runInfo, "chi", 90.0);
    addTimeSeriesEntry(runInfo, "omega", 0.0);
    Goniometer gm;
    gm.makeUniversalGoniometer();
    runInfo.setGoniometer(gm, true);

    DblMatrix r = runInfo.getGoniometerMatrix();
    V3D rot = r * V3D(-1, 0, 0);
    TS_ASSERT_EQUALS(rot, V3D(0, -sqrt(0.5), sqrt(0.5)));
  }

  /** Save and load to NXS file */
  void test_nexus() {
    NexusTestHelper th(true);
    th.createFile("RunTest.nxs");

    Run run1;
    addTimeSeriesEntry(run1, "double_series", 45.0);
    run1.addProperty(new PropertyWithValue<int>("int_val", 1234));
    run1.addProperty(new PropertyWithValue<std::string>(
        "string_val", "help_im_stuck_in_a_log_file"));
    run1.addProperty(new PropertyWithValue<double>("double_val", 5678.9));
    addTimeSeriesEntry(run1, "phi", 12.3);
    addTimeSeriesEntry(run1, "chi", 45.6);
    addTimeSeriesEntry(run1, "omega", 78.9);
    addTimeSeriesEntry(run1, "proton_charge", 78.9);

    Goniometer gm;
    gm.makeUniversalGoniometer();
    run1.setGoniometer(gm, true);

    run1.storeHistogramBinBoundaries(m_test_energy_bins);

    run1.saveNexus(th.file.get(), "logs");
    th.file->openGroup("logs", "NXgroup");
    th.file->makeGroup("junk_to_ignore", "NXmaterial");
    th.file->makeGroup("more_junk_to_ignore", "NXsample");

    // ---- Now re-load the same and compare ------
    th.reopenFile();
    Run run2;
    run2.loadNexus(th.file.get(), "logs");
    TS_ASSERT(run2.hasProperty("double_series"));
    TS_ASSERT(run2.hasProperty("int_val"));
    TS_ASSERT(run2.hasProperty("string_val"));
    TS_ASSERT(run2.hasProperty("double_val"));
    // This test both uses the goniometer axes AND looks up some values.
    TS_ASSERT_EQUALS(run2.getGoniometerMatrix(), run1.getGoniometerMatrix());

    std::pair<double, double> edges(0.0, 0.0);
    TS_ASSERT_THROWS_NOTHING(edges = run2.histogramBinBoundaries(1.2));
    TS_ASSERT_DELTA(edges.first, 0.7, 1e-12);
    TS_ASSERT_DELTA(edges.second, 1.6, 1e-12);

    // Reload without opening the group (for backwards-compatible reading of old
    // files)
    Run run3;
    th.file->openGroup("logs", "NXgroup");
    run3.loadNexus(th.file.get(), "");
    TS_ASSERT(run3.hasProperty("double_series"));
    TS_ASSERT(run3.hasProperty("int_val"));
    TS_ASSERT(run3.hasProperty("string_val"));
    TS_ASSERT(run3.hasProperty("double_val"));
  }

  /** Save and load to NXS file */
  void test_nexus_multiple_goniometer() {
    NexusTestHelper th(true);
    th.createFile("RunTest2.nxs");

    Run run1;
    addTimeSeriesEntry(run1, "phi", 12.3);
    addTimeSeriesEntry(run1, "chi", 45.6);
    addTimeSeriesEntry(run1, "omega", 78.9);
    addTimeSeriesEntry(run1, "proton_charge", 78.9);

    Goniometer gm;
    gm.makeUniversalGoniometer();
    run1.setGoniometer(gm, true);
    run1.addGoniometer(run1.getGoniometer(0));
    run1.addGoniometer(Goniometer());

    run1.saveNexus(th.file.get(), "logs");
    th.file->openGroup("logs", "NXgroup");

    // ---- Now re-load the same and compare ------
    th.reopenFile();
    Run run2;
    run2.loadNexus(th.file.get(), "logs");
    TS_ASSERT_EQUALS(run2, run1);
  }

  void test_setGoniometerWithLogsUsesTimeSeriesAverage() {
    Run run;

    auto tsp = std::make_unique<TimeSeriesProperty<double>>("omega");
    tsp->addValue("2018-01-01T00:00:00", 90.0);
    tsp->addValue("2018-01-01T00:00:01", 0.0);
    tsp->addValue("2018-01-01T02:00:00", 0.0);
    tsp->addValue("2018-01-01T03:00:00", 0.0);
    run.addProperty(tsp.release());

    addTimeSeriesEntry(run, "chi", 12.3);
    addTimeSeriesEntry(run, "phi", 45.6);

    Goniometer gm;
    gm.makeUniversalGoniometer();
    run.setGoniometer(gm, true);
    gm = run.getGoniometer();

    TS_ASSERT_DELTA(gm.getAxis(0).angle, 0.0, 1e-2);
    TS_ASSERT_EQUALS(gm.getAxis(1).angle, 12.3);
    TS_ASSERT_EQUALS(gm.getAxis(2).angle, 45.6);
  }

  void test_setGoniometersWithLogsUsesTimeSeries() {
    Run run;

    auto omega = std::make_unique<TimeSeriesProperty<double>>("omega");
    omega->addValue("2018-01-01T00:00:00", 0.0);
    omega->addValue("2018-01-01T01:00:00", 90.0);
    omega->addValue("2018-01-01T02:00:00", 0.0);
    run.addProperty(omega.release());

    auto chi = std::make_unique<TimeSeriesProperty<double>>("chi");
    chi->addValue("2018-01-01T00:00:00", 0.0);
    chi->addValue("2018-01-01T01:00:00", 0.0);
    chi->addValue("2018-01-01T02:00:00", 0.0);
    run.addProperty(chi.release());

    addTimeSeriesEntry(run, "phi", 0.0);

    Goniometer gm;
    gm.makeUniversalGoniometer();
    TS_ASSERT_THROWS(run.setGoniometers(gm), const std::runtime_error &);

    auto phi = std::make_unique<TimeSeriesProperty<double>>("phi");
    phi->addValue("2018-01-01T00:00:00", 0.0);
    phi->addValue("2018-01-01T01:00:00", 0.0);
    phi->addValue("2018-01-01T02:00:00", 90.0);
    run.addProperty(phi.release(), true);

    run.setGoniometers(gm);

    TS_ASSERT_EQUALS(run.getNumGoniometers(), 3);

    gm = run.getGoniometer(0);
    TS_ASSERT_DELTA(gm.getAxis(0).angle, 0.0, 1e-7);
    TS_ASSERT_DELTA(gm.getAxis(1).angle, 0.0, 1e-7);
    TS_ASSERT_DELTA(gm.getAxis(2).angle, 0.0, 1e-7);

    gm = run.getGoniometer(1);
    TS_ASSERT_DELTA(gm.getAxis(0).angle, 90.0, 1e-7);
    TS_ASSERT_DELTA(gm.getAxis(1).angle, 0.0, 1e-7);
    TS_ASSERT_DELTA(gm.getAxis(2).angle, 0.0, 1e-7);

    gm = run.getGoniometer(2);
    TS_ASSERT_DELTA(gm.getAxis(0).angle, 0.0, 1e-7);
    TS_ASSERT_DELTA(gm.getAxis(1).angle, 0.0, 1e-7);
    TS_ASSERT_DELTA(gm.getAxis(2).angle, 90.0, 1e-7);
  }

  /** Check for loading the old way of saving proton_charge */
  void test_legacy_nexus() {
    NexusTestHelper th(true);
    th.createFile("RunTest.nxs");
    th.file->makeGroup("sample", "NXsample", 1);
    th.file->writeData("proton_charge", 1.234);
    th.reopenFile();
    th.file->openGroup("sample", "NXsample");
    Run run3;
    run3.loadNexus(th.file.get(), "");

    TS_ASSERT_DELTA(run3.getProtonCharge(), 1.234, 1e-5);
  }

  void test_equals_when_runs_empty() {
    Run a{};
    Run b{a};
    TS_ASSERT_EQUALS(a, b);
  }

  void test_equals_when_runs_identical() {
    Run a{};
    a.addProperty(std::make_unique<ConcreteProperty>());
    const DblMatrix rotation_x{
        {1, 0, 0, 0, 0, -1, 0, 1, 0}}; // 90 degrees around x axis
    a.setGoniometer(Goniometer{rotation_x}, false /*do not use log angles*/);
    a.storeHistogramBinBoundaries({1, 2, 3, 4});
    Run b{a};
    TS_ASSERT_EQUALS(a, b);
    TS_ASSERT(!(a != b));
  }

  void test_not_equal_when_histogram_bin_boundaries_differ() {
    Run a{};
    a.addProperty(std::make_unique<ConcreteProperty>());
    DblMatrix rotation_x{
        {1, 0, 0, 0, 0, -1, 0, 1, 0}}; // 90 degrees around x axis
    a.setGoniometer(Goniometer{rotation_x}, false /*do not use log angles*/);
    a.storeHistogramBinBoundaries({1, 2, 3, 4});
    Run b{a};
    b.storeHistogramBinBoundaries({0, 2, 3, 4});
    TS_ASSERT_DIFFERS(a, b);
    TS_ASSERT(!(a == b));
  }

  void test_not_equal_when_properties_differ() {
    Run a{};
    a.addProperty(std::make_unique<ConcreteProperty>());
    DblMatrix rotation_x{
        {1, 0, 0, 0, 0, -1, 0, 1, 0}}; // 90 degrees around x axis
    a.setGoniometer(Goniometer{rotation_x}, false /*do not use log angles*/);
    a.storeHistogramBinBoundaries({1, 2, 3, 4});
    Run b{a};
    b.removeProperty("Test");
    TS_ASSERT_DIFFERS(a, b);
    TS_ASSERT(!(a == b));
  }

  void test_not_equal_when_goniometer_differ() {
    Run a{};
    a.addProperty(std::make_unique<ConcreteProperty>());
    DblMatrix rotation_x{
        {1, 0, 0, 0, 0, -1, 0, 1, 0}}; // 90 degrees around x axis
    a.setGoniometer(Goniometer{rotation_x}, false /*do not use log angles*/);
    a.storeHistogramBinBoundaries({1, 2, 3, 4});
    Run b{a};
    Mantid::Kernel::DblMatrix rotation_y{
        {0, 0, 1, 0, 1, 0, -1, 0, 0}}; // 90 degrees around y axis
    b.setGoniometer(Goniometer{rotation_y}, false /*do not use log angles*/);
    TS_ASSERT_DIFFERS(a, b);
    TS_ASSERT(!(a == b));
  }

private:
  /// Testing bins
  std::vector<double> m_test_energy_bins;
};

//---------------------------------------------------------------------------------------
// Performance test
//---------------------------------------------------------------------------------------

class RunTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RunTestPerformance *createSuite() { return new RunTestPerformance(); }
  static void destroySuite(RunTestPerformance *suite) { delete suite; }

  RunTestPerformance() : m_testRun(), m_propName("test") {
    addTestTimeSeries(m_testRun, m_propName);
  }

  void test_Accessing_Single_Value_From_Times_Series_A_Large_Number_Of_Times() {
    for (size_t i = 0; i < 20000; ++i) {
      m_testRun.getPropertyAsSingleValue(m_propName);
    }
  }

  Run m_testRun;
  std::string m_propName;
};
