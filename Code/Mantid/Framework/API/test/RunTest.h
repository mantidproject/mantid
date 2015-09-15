#ifndef RUNTEST_H_
#define RUNTEST_H_

#include "MantidAPI/Run.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/NexusTestHelper.h"
#include "MantidGeometry/Instrument/Goniometer.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// Helper class
namespace
{
  class ConcreteProperty : public Property
  {
  public:
    ConcreteProperty() : Property( "Test", typeid( int ) ) {}
    ConcreteProperty* clone() const { return new ConcreteProperty(*this); }
    bool isDefault() const { return true; }
    std::string getDefault() const { return "getDefault() is not implemented in this class"; }
    std::string value() const { return "Nothing"; }
    std::string setValue( const std::string& ) { return ""; }
    std::string setValueFromProperty( const Property& ) { return ""; }
    std::string setDataItem(const boost::shared_ptr<DataItem>) { return ""; } 
    Property& operator+=( Property const * ) { return *this; }
  };

  void addTestTimeSeries(Run & run, const std::string & name)
  {
    auto timeSeries = new TimeSeriesProperty<double>(name);
    timeSeries->addValue("2012-07-19T16:17:00",2);
    timeSeries->addValue("2012-07-19T16:17:10",3);
    timeSeries->addValue("2012-07-19T16:17:20",4);
    timeSeries->addValue("2012-07-19T16:17:30",5);
    timeSeries->addValue("2012-07-19T16:17:40",6);
    timeSeries->addValue("2012-07-19T16:17:50",20);
    timeSeries->addValue("2012-07-19T16:18:00",21);
    timeSeries->addValue("2012-07-19T16:18:10",22);
    timeSeries->addValue("2012-07-19T16:19:20",23);
    timeSeries->addValue("2012-07-19T16:19:20",24);
    run.addProperty(timeSeries);
  }
}


class RunTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RunTest *createSuite() { return new RunTest(); }
  static void destroySuite( RunTest *suite ) { delete suite; }

  RunTest() : m_test_energy_bins(5)
  {
    m_test_energy_bins[0] = -1.1;
    m_test_energy_bins[1] = -0.2;
    m_test_energy_bins[2] = 0.7;
    m_test_energy_bins[3] = 1.6;
    m_test_energy_bins[4] = 3.2;
  }

  void testAddGetData()
  {
    Run runInfo;

    Property *p = new ConcreteProperty();
    TS_ASSERT_THROWS_NOTHING( runInfo.addProperty(p) );

    Property *pp = NULL;
    TS_ASSERT_THROWS_NOTHING( pp = runInfo.getProperty("Test") );
    TS_ASSERT_EQUALS( p, pp );
    TS_ASSERT( ! pp->name().compare("Test") );
    TS_ASSERT( dynamic_cast<ConcreteProperty*>(pp) );
    TS_ASSERT_THROWS( pp = runInfo.getProperty("NotThere"), Exception::NotFoundError );

    std::vector< Property* > props = runInfo.getProperties();
    TS_ASSERT( ! props.empty() );
    TS_ASSERT_EQUALS( props.size(), 1 );
    TS_ASSERT( ! props[0]->name().compare("Test") );
    TS_ASSERT( dynamic_cast<ConcreteProperty*>(props[0]) );
  }

  void testRemoveLogData()
  {
    Run runInfo;
    
    Property *p = new ConcreteProperty();
    TS_ASSERT_THROWS_NOTHING( runInfo.addProperty(p) );
    TS_ASSERT_THROWS_NOTHING( runInfo.removeProperty("Test") );
    TS_ASSERT_EQUALS( runInfo.getProperties().size(), 0 );
  }

  void testGetSetProtonCharge()
  {
    Run runInfo;
    TS_ASSERT_THROWS(runInfo.getProtonCharge(), Exception::NotFoundError);
    TS_ASSERT_THROWS_NOTHING( runInfo.setProtonCharge(10.0) );
    TS_ASSERT_EQUALS( runInfo.getProtonCharge(), 10.0 );
  }

  void testCopyAndAssignment()
  {
    Run runInfo;
    runInfo.setProtonCharge(10.0);
    Property *p = new ConcreteProperty();
    TS_ASSERT_THROWS_NOTHING( runInfo.addProperty(p) );
    TS_ASSERT_EQUALS( runInfo.getProperties().size(), 2);
    
    //Copy constructor
    Run runInfo_2(runInfo);
    TS_ASSERT_EQUALS( runInfo_2.getProperties().size(), 2);
    TS_ASSERT_DELTA( runInfo_2.getProtonCharge(), 10.0, 1e-8);
    TS_ASSERT_EQUALS( runInfo_2.getLogData("Test")->value(), "Nothing" );    


    // Now assignment
    runInfo.setProtonCharge(15.0);
    runInfo.removeProperty("Test");
    runInfo_2 = runInfo;
    TS_ASSERT_EQUALS( runInfo_2.getProperties().size(), 1);
    TS_ASSERT_DELTA( runInfo_2.getProtonCharge(), 15.0, 1e-8);
  }

  void testMemory()
  {
    Run runInfo;
    TS_ASSERT_EQUALS( runInfo.getMemorySize(), sizeof(Goniometer));
    
    Property *p = new ConcreteProperty();
    runInfo.addProperty(p);
    size_t classSize = sizeof(ConcreteProperty) + sizeof( void *)+sizeof(Goniometer);
    TS_ASSERT_EQUALS(classSize, runInfo.getMemorySize());
  }

  void test_GetTimeSeriesProperty_Returns_TSP_When_Log_Exists()
  {
    Run runInfo;
    const std::string & name = "double_time_series";
    const double value = 10.9;
    addTimeSeriesEntry(runInfo, name, value);

    TimeSeriesProperty<double> * tsp(NULL);
    TS_ASSERT_THROWS_NOTHING(tsp = runInfo.getTimeSeriesProperty<double>(name));
    TS_ASSERT_DELTA(tsp->firstValue(), value, 1e-12);
  }

  void test_GetTimeSeriesProperty_Throws_When_Log_Does_Not_Exist()
  {
    Run runInfo;
    TS_ASSERT_THROWS(runInfo.getTimeSeriesProperty<double>("not_a_log"), Exception::NotFoundError);
  }

  void test_GetTimeSeriesProperty_Throws_When_Log_Exists_But_Is_Not_Correct_Type()
  {
    Run runInfo;
    const std::string & name = "double_prop";
    runInfo.addProperty(name, 5.6); // Standard double property

    TS_ASSERT_THROWS(runInfo.getTimeSeriesProperty<double>(name), std::invalid_argument);
  }

  void test_GetPropertyAsType_Throws_When_Property_Does_Not_Exist()
  {
    Run runInfo;
    TS_ASSERT_THROWS(runInfo.getPropertyValueAsType<double>("not_a_log"), Exception::NotFoundError);
  }

  void test_GetPropertyAsType_Returns_Expected_Value_When_Type_Is_Correct()
  {
    Run runInfo;
    const std::string & name = "double_prop";
    const double value = 5.6;
    runInfo.addProperty(name, value); // Standard double property

    double retrieved(0.0);
    TS_ASSERT_THROWS_NOTHING(retrieved = runInfo.getPropertyValueAsType<double>(name));
    TS_ASSERT_DELTA(retrieved, value, 1e-12);
  }

  void test_GetPropertyAsType_Throws_When_Requested_Type_Does_Not_Match()
  {
    Run runInfo;
    runInfo.addProperty("double_prop", 6.7); // Standard double property

    TS_ASSERT_THROWS(runInfo.getPropertyValueAsType<int>("double_prop"), std::invalid_argument);
  }

  void test_GetPropertyAsSingleValue_Throws_If_Type_Is_Not_Double_Or_TimeSeries_Double()
  {
    Run runInfo;
    const std::string name = "int_prop";
    runInfo.addProperty(name, 1); // Adds an int property

    TS_ASSERT_THROWS(runInfo.getPropertyAsSingleValue(name), std::invalid_argument);
  }

  void test_GetPropertyAsSingleValue_Returns_Simple_Mean_By_Default_For_Time_Series()
  {
    Run runInfo;
    const std::string name = "series";
    addTestTimeSeries(runInfo, name);

    const double expectedValue(13.0);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name), expectedValue, 1e-12);
  }

  void test_GetPropertyAsSingleValue_Returns_Correct_SingleValue_For_Each_StatisticType()
  {
    Run runInfo;
    const std::string name = "series";
    addTestTimeSeries(runInfo, name);
    
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::Mean), 13.0, 1e-12);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::Minimum), 2.0, 1e-12);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::Maximum), 24.0, 1e-12);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::FirstValue), 2.0, 1e-12);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::LastValue), 24.0, 1e-12);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::Median), 13.0, 1e-12);
  }

  void test_GetPropertyAsSingleValue_Returns_Expected_Single_Value_On_Successive_Calls_With_Different_Stat_Types()
  {
    Run run;
    const std::string name = "series";
    addTestTimeSeries(run, name);

    TS_ASSERT_EQUALS(run.getPropertyAsSingleValue(name,Math::Mean), 13.0);
    TS_ASSERT_EQUALS(run.getPropertyAsSingleValue(name,Math::Mean), 13.0);
    TS_ASSERT_EQUALS(run.getPropertyAsSingleValue(name,Math::Minimum), 2.0);
    TS_ASSERT_EQUALS(run.getPropertyAsSingleValue(name,Math::Minimum), 2.0);
  }


  void test_GetPropertyAsSingleValue_Returns_Correct_Value_On_Second_Call_When_Log_Has_Been_Replaced()
  {
    Run runInfo;
    const std::string name = "double";
    double value(5.1);
    runInfo.addProperty(name, value);

    TS_ASSERT_EQUALS(runInfo.getPropertyAsSingleValue(name), value);

    // Replace the log with a different value
    value = 10.3;
    runInfo.addProperty(name, value, /*overwrite*/true);

    TS_ASSERT_EQUALS(runInfo.getPropertyAsSingleValue(name), value);
  }


  void test_storeHistogramBinBoundaries_Throws_If_Fewer_Than_Two_Values_Are_Given()
  {
    Run runInfo;

    std::vector<double> bins;
    TS_ASSERT_THROWS(runInfo.storeHistogramBinBoundaries(bins), std::invalid_argument);
    bins.push_back(0.5);
    TS_ASSERT_THROWS(runInfo.storeHistogramBinBoundaries(bins), std::invalid_argument);
    bins.push_back(1.5);
    TS_ASSERT_THROWS_NOTHING(runInfo.storeHistogramBinBoundaries(bins));
  }

  void test_storeHistogramBinBoundaries_Throws_If_First_Value_Is_Greater_Or_Equal_To_Last_Value()
  {
    Run runInfo;
    std::vector<double> bins(2, 0.0);

    TS_ASSERT_THROWS(runInfo.storeHistogramBinBoundaries(bins), std::out_of_range);

    bins[0] = -1.5;
    bins[1] = -1.5;
    TS_ASSERT_THROWS(runInfo.storeHistogramBinBoundaries(bins), std::out_of_range);

    bins[0] = 2.1;
    bins[1] = 2.1;
    TS_ASSERT_THROWS(runInfo.storeHistogramBinBoundaries(bins), std::out_of_range);

    bins[0] = -1.5;
    bins[1] = -1.6;
    TS_ASSERT_THROWS(runInfo.storeHistogramBinBoundaries(bins), std::out_of_range);

    bins[0] = 2.1;
    bins[1] = 1.9;
    TS_ASSERT_THROWS(runInfo.storeHistogramBinBoundaries(bins), std::out_of_range);
  }

  void test_storeHistogramBinBoundaries_Succeeds_With_Valid_Bins()
  {
    Run runInfo;

    TS_ASSERT_THROWS_NOTHING(runInfo.storeHistogramBinBoundaries(m_test_energy_bins));
    TS_ASSERT_THROWS_NOTHING(runInfo.histogramBinBoundaries(m_test_energy_bins[1] + 0.1));
  }

  void test_histogramBinBoundaries_Throws_RuntimeError_For_New_Run()
  {
    Run runInfo;

    TS_ASSERT_THROWS(runInfo.histogramBinBoundaries(1.5), std::runtime_error);
  }

  void test_histogramBinBoundaries_Throws_RuntimeError_When_Value_Is_Outside_Boundaries_Range()
  {
    using namespace Mantid::Kernel;
    Run runInfo;
    runInfo.storeHistogramBinBoundaries(m_test_energy_bins);

    TS_ASSERT_THROWS(runInfo.histogramBinBoundaries(m_test_energy_bins.front() - 1.3), std::out_of_range);
    TS_ASSERT_THROWS(runInfo.histogramBinBoundaries(m_test_energy_bins.back() + 1.3), std::out_of_range);
  }

  void test_histogramBinBoundaries_Returns_Closest_Lower_And_Upper_Boundary_For_Valid_Bin_Value_Away_From_Any_Edge()
  {
    using namespace Mantid::Kernel;
    Run runInfo;
    runInfo.storeHistogramBinBoundaries(m_test_energy_bins);

    std::pair<double,double> edges;
    TS_ASSERT_THROWS_NOTHING(edges = runInfo.histogramBinBoundaries(1.2));

    TS_ASSERT_DELTA(edges.first, 0.7, 1e-12);
    TS_ASSERT_DELTA(edges.second, 1.6, 1e-12);
  }

  void test_histogramBinBoundaries_Returns_The_Value_And_Next_Boundary_Along_If_Given_Value_Equals_A_Bin_Edge_Away_From_Ends()
  {
    using namespace Mantid::Kernel;
    Run runInfo;
    runInfo.storeHistogramBinBoundaries(m_test_energy_bins);

    std::pair<double,double> edges;
    TS_ASSERT_THROWS_NOTHING(edges = runInfo.histogramBinBoundaries(-0.2));
    TS_ASSERT_DELTA(edges.first, -0.2, 1e-12);
    TS_ASSERT_DELTA(edges.second, 0.7, 1e-12);
  }

  void test_histogramBinBoundaries_Returns_The_Value_And_Next_Boundary_Along_If_Given_Value_Equals_A_The_First_Bin_Edge()
  {
    using namespace Mantid::Kernel;
    Run runInfo;
    runInfo.storeHistogramBinBoundaries(m_test_energy_bins);

    std::pair<double,double> edges;
    TS_ASSERT_THROWS_NOTHING(edges = runInfo.histogramBinBoundaries(m_test_energy_bins.front()));
    TS_ASSERT_DELTA(edges.first, -1.1, 1e-12);
    TS_ASSERT_DELTA(edges.second, -0.2, 1e-12);
  }

  void test_histogramBinBoundaries_Returns_The_Value_And_Previous_Boundary_If_Given_Value_Equals_The_Last_Bin_Edge()
  {
    using namespace Mantid::Kernel;
    Run runInfo;
    runInfo.storeHistogramBinBoundaries(m_test_energy_bins);

    std::pair<double,double> edges;
    TS_ASSERT_THROWS_NOTHING(edges = runInfo.histogramBinBoundaries(m_test_energy_bins.back()));
    TS_ASSERT_DELTA(edges.first, 1.6, 1e-12);
    TS_ASSERT_DELTA(edges.second, 3.2, 1e-12);
  }

  void test_getBinBoundaries()
  {
      using namespace Mantid::Kernel;
      Run runInfo;
      runInfo.storeHistogramBinBoundaries(m_test_energy_bins);

      std::vector<double> bounds;
      TS_ASSERT_THROWS_NOTHING(bounds = runInfo.getBinBoundaries());
      for(size_t i=0;i<m_test_energy_bins.size();++i)
      {
          TS_ASSERT_DELTA(bounds.at(i),m_test_energy_bins.at(i), 1e-12);
      }

  }

  void test_getGoniometer()
  {
    Run runInfo;
    TS_ASSERT_THROWS_NOTHING( runInfo.getGoniometer() );
    // No axes by default
    TS_ASSERT_EQUALS( runInfo.getGoniometer().getNumberAxes(), 0 );
    // Now does copy work?
    Goniometer gm;
    gm.makeUniversalGoniometer();
    runInfo.setGoniometer(gm, false);
    TS_ASSERT_EQUALS( runInfo.getGoniometer().getNumberAxes(), 3 );
    Run runCopy(runInfo);
    TS_ASSERT_EQUALS( runCopy.getGoniometer().getNumberAxes(), 3 );
    runCopy = runInfo;
    TS_ASSERT_EQUALS( runCopy.getGoniometer().getNumberAxes(), 3 );
  }


  void addTimeSeriesEntry(Run & runInfo, std::string name, double val)
  {
    TimeSeriesProperty<double> * tsp;
    tsp = new TimeSeriesProperty<double>(name);
    tsp->addValue("2011-05-24T00:00:00", val);
    runInfo.addProperty(tsp);
  }

  void test_clear()
  {
    // Set up a Run object with 3 properties in it (1 time series, 2 single value)
    Run runInfo;
    const std::string stringProp("aStringProp");
    const std::string stringVal("testing");
    runInfo.addProperty(stringProp,stringVal);
    const std::string intProp("anIntProp");
    runInfo.addProperty(intProp,99);
    const std::string tspProp("tsp");
    addTestTimeSeries(runInfo,"tsp");

    // Check it's set up right
    TS_ASSERT_EQUALS( runInfo.getProperties().size(), 3 );
    auto tsp = runInfo.getTimeSeriesProperty<double>(tspProp);
    TS_ASSERT_EQUALS( tsp->realSize(), 10 )

    // Do the clearing work
    TS_ASSERT_THROWS_NOTHING( runInfo.clearTimeSeriesLogs() );

    // Check the time-series property is empty, but not the others
    TS_ASSERT_EQUALS( runInfo.getProperties().size(), 3 );
    TS_ASSERT_EQUALS( tsp->realSize(), 0 )
    TS_ASSERT_EQUALS( runInfo.getPropertyValueAsType<std::string>(stringProp), stringVal );
    TS_ASSERT_EQUALS( runInfo.getPropertyValueAsType<int>(intProp), 99 );
  }

  /** Setting up a goniometer and the angles to feed it
   * using sample logs, then getting the right rotation matrix out.
   */
  void test_setGoniometerWhenLogsDoNotExistsThrows()
  {
    Run runInfo;
    Goniometer gm;
    gm.makeUniversalGoniometer();

    TS_ASSERT_THROWS(runInfo.setGoniometer(gm, true), std::runtime_error);
  }

  /** Setting up a goniometer and the angles to feed it
   * using sample logs, then getting the right rotation matrix out.
   */
  void test_setGoniometer_Not_Using_Logs_Preserves_Input()
  {
    Run runInfo;
    DblMatrix rotation(3,3, true);
    Goniometer gm(rotation);

    TS_ASSERT_EQUALS(runInfo.getGoniometer().getNumberAxes(), 0);
    TS_ASSERT_EQUALS(runInfo.getGoniometer().getR(), rotation);
  }

  /** Setting up a goniometer and the angles to feed it
   * using sample logs, then getting the right rotation matrix out.
   */
  void test_getGoniometerMatrix()
  {
    Run runInfo;
    addTimeSeriesEntry(runInfo, "phi", 90.0);
    addTimeSeriesEntry(runInfo, "chi", 90.0);
    addTimeSeriesEntry(runInfo, "omega", 90.0);
    Goniometer gm;
    gm.makeUniversalGoniometer();
    runInfo.setGoniometer(gm, true);
    DblMatrix r = runInfo.getGoniometerMatrix();
    V3D rot = r * V3D(-1,0,0);
    TS_ASSERT_EQUALS( rot, V3D(1, 0, 0));
    rot = r * V3D(0,0,1);
    TS_ASSERT_EQUALS( rot, V3D(0, 1, 0));
  }

  void test_getGoniometerMatrix2()
  {
    Run runInfo;
    addTimeSeriesEntry(runInfo, "phi", 45.0);
    addTimeSeriesEntry(runInfo, "chi", 90.0);
    addTimeSeriesEntry(runInfo, "omega", 0.0);
    Goniometer gm;
    gm.makeUniversalGoniometer();
    runInfo.setGoniometer(gm, true);

    DblMatrix r = runInfo.getGoniometerMatrix();
    V3D rot = r * V3D(-1,0,0);
    TS_ASSERT_EQUALS( rot, V3D(0, -sqrt(0.5), sqrt(0.5)));
  }


  /** Save and load to NXS file */
  void test_nexus()
  {
    NexusTestHelper th(true);
    th.createFile("RunTest.nxs");

    Run run1;
    addTimeSeriesEntry(run1, "double_series", 45.0);
    run1.addProperty( new PropertyWithValue<int>("int_val", 1234) );
    run1.addProperty( new PropertyWithValue<std::string>("string_val", "help_im_stuck_in_a_log_file") );
    run1.addProperty( new PropertyWithValue<double>("double_val", 5678.9) );
    addTimeSeriesEntry(run1, "phi", 12.3);
    addTimeSeriesEntry(run1, "chi", 45.6);
    addTimeSeriesEntry(run1, "omega", 78.9);
    addTimeSeriesEntry(run1, "proton_charge", 78.9);

    Goniometer gm;
    gm.makeUniversalGoniometer();
    run1.setGoniometer(gm, true);

    run1.storeHistogramBinBoundaries(m_test_energy_bins);

    run1.saveNexus(th.file, "logs");
    th.file->openGroup("logs", "NXgroup");
    th.file->makeGroup("junk_to_ignore", "NXmaterial");
    th.file->makeGroup("more_junk_to_ignore", "NXsample");

    // ---- Now re-load the same and compare ------
    th.reopenFile();
    Run run2;
    run2.loadNexus(th.file, "logs");
    TS_ASSERT( run2.hasProperty("double_series") );
    TS_ASSERT( run2.hasProperty("int_val") );
    TS_ASSERT( run2.hasProperty("string_val") );
    TS_ASSERT( run2.hasProperty("double_val") );
    // This test both uses the goniometer axes AND looks up some values.
    TS_ASSERT_EQUALS( run2.getGoniometerMatrix(), run1.getGoniometerMatrix() );

    std::pair<double,double> edges(0.0,0.0);
    TS_ASSERT_THROWS_NOTHING(edges = run2.histogramBinBoundaries(1.2));
    TS_ASSERT_DELTA(edges.first, 0.7, 1e-12);
    TS_ASSERT_DELTA(edges.second, 1.6, 1e-12);

    // Reload without opening the group (for backwards-compatible reading of old files)
    Run run3;
    th.file->openGroup("logs", "NXgroup");
    run3.loadNexus(th.file, "");
    TS_ASSERT( run3.hasProperty("double_series") );
    TS_ASSERT( run3.hasProperty("int_val") );
    TS_ASSERT( run3.hasProperty("string_val") );
    TS_ASSERT( run3.hasProperty("double_val") );
  }

  /** Check for loading the old way of saving proton_charge */
  void test_legacy_nexus()
  {
    NexusTestHelper th(true);
    th.createFile("RunTest.nxs");
    th.file->makeGroup("sample", "NXsample", 1);
    th.file->writeData("proton_charge", 1.234);
    th.reopenFile();
    th.file->openGroup("sample", "NXsample");
    Run run3;
    run3.loadNexus(th.file, "");

    TS_ASSERT_DELTA( run3.getProtonCharge(), 1.234, 1e-5 );
  }

private:
  /// Testing bins
  std::vector<double> m_test_energy_bins;
};

//---------------------------------------------------------------------------------------
// Performance test
//---------------------------------------------------------------------------------------

class RunTestPerformance : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RunTestPerformance *createSuite() { return new RunTestPerformance(); }
  static void destroySuite( RunTestPerformance *suite ) { delete suite; }

  RunTestPerformance() : m_testRun(), m_propName("test")
  {
    addTestTimeSeries(m_testRun, m_propName);
  }

  void test_Accessing_Single_Value_From_Times_Series_A_Large_Number_Of_Times()
  {
    double value(0.0);
    for(size_t i = 0; i < 20000; ++i)
    {
      value = m_testRun.getPropertyAsSingleValue(m_propName);
    }
    // Enure variable is used so that it is not optimised away by the compiler
    value += 1.0;
  }

  Run m_testRun;
  std::string m_propName;
};


#endif
