#ifndef LOG_MANAGER_TEST_H_
#define LOG_MANAGER_TEST_H_

#include "MantidAPI/LogManager.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>
#include "MantidKernel/NexusTestHelper.h"
#include "MantidGeometry/Instrument/Goniometer.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using Mantid::Kernel::NexusTestHelper;

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

  void addTestTimeSeries(LogManager & run, const std::string & name)
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
 
  void addTimeSeriesEntry(LogManager & runInfo, std::string name, double val)
  {
    TimeSeriesProperty<double> * tsp;
    tsp = new TimeSeriesProperty<double>(name);
    tsp->addValue("2011-05-24T00:00:00", val);
    runInfo.addProperty(tsp);
  }

class LogManagerTest : public CxxTest::TestSuite
{
public:
  void testAddGetData()
  {
    LogManager runInfo;

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
    LogManager runInfo;
    
    Property *p = new ConcreteProperty();
    TS_ASSERT_THROWS_NOTHING( runInfo.addProperty(p) );
    TS_ASSERT_THROWS_NOTHING( runInfo.removeProperty("Test") );
    TS_ASSERT_EQUALS( runInfo.getProperties().size(), 0 );
  }

 

  
  void testMemory()
  {
    LogManager runInfo;
    TS_ASSERT_EQUALS( runInfo.getMemorySize(), 0);
    
    Property *p = new ConcreteProperty();
    runInfo.addProperty(p);

    TS_ASSERT_EQUALS( runInfo.getMemorySize(), sizeof(ConcreteProperty) + sizeof( void *));
  }

  void test_GetTimeSeriesProperty_Returns_TSP_When_Log_Exists()
  {
    LogManager runInfo;
    const std::string & name = "double_time_series";
    const double value = 10.9;
    addTimeSeriesEntry(runInfo, name, value);

    TimeSeriesProperty<double> * tsp(NULL);
    TS_ASSERT_THROWS_NOTHING(tsp = runInfo.getTimeSeriesProperty<double>(name));
    TS_ASSERT_DELTA(tsp->firstValue(), value, 1e-12);
  }

  void test_GetTimeSeriesProperty_Throws_When_Log_Does_Not_Exist()
  {
    LogManager runInfo;
    TS_ASSERT_THROWS(runInfo.getTimeSeriesProperty<double>("not_a_log"), Exception::NotFoundError);
  }

  void test_GetTimeSeriesProperty_Throws_When_Log_Exists_But_Is_Not_Correct_Type()
  {
    LogManager runInfo;
    const std::string & name = "double_prop";
    runInfo.addProperty(name, 5.6); // Standard double property

    TS_ASSERT_THROWS(runInfo.getTimeSeriesProperty<double>(name), std::invalid_argument);
  }

  void test_GetPropertyAsType_Throws_When_Property_Does_Not_Exist()
  {
    LogManager runInfo;
    TS_ASSERT_THROWS(runInfo.getPropertyValueAsType<double>("not_a_log"), Exception::NotFoundError);
  }

  void test_GetPropertyAsType_Returns_Expected_Value_When_Type_Is_Correct()
  {
    LogManager runInfo;
    const std::string & name = "double_prop";
    const double value = 5.6;
    runInfo.addProperty(name, value); // Standard double property

    double retrieved(0.0);
    TS_ASSERT_THROWS_NOTHING(retrieved = runInfo.getPropertyValueAsType<double>(name));
    TS_ASSERT_DELTA(retrieved, value, 1e-12);
  }

  void test_GetPropertyAsType_Throws_When_Requested_Type_Does_Not_Match()
  {
    LogManager runInfo;
    runInfo.addProperty("double_prop", 6.7); // Standard double property

    TS_ASSERT_THROWS(runInfo.getPropertyValueAsType<int>("double_prop"), std::invalid_argument);
  }

  void test_GetPropertyAsSingleValue_Throws_If_Type_Is_Not_Double_Or_TimeSeries_Double()
  {
    LogManager runInfo;
    const std::string name = "int_prop";
    runInfo.addProperty(name, 1); // Adds an int property

    TS_ASSERT_THROWS(runInfo.getPropertyAsSingleValue(name), std::invalid_argument);
  }

  void test_GetPropertyAsSingleValue_Throws_If_StatisticType_Is_Unknown_And_Type_Is_TimeSeries()
  {
    LogManager runInfo;
    const std::string name = "series";
    addTestTimeSeries(runInfo, name);

    const unsigned int statistic(100);
    TS_ASSERT_THROWS(runInfo.getPropertyAsSingleValue(name, (Math::StatisticType)statistic), std::invalid_argument);
  }

  void test_GetPropertyAsSingleValue_Returns_Simple_Mean_By_Default_For_Time_Series()
  {
    LogManager runInfo;
    const std::string name = "series";
    addTestTimeSeries(runInfo, name);

    const double expectedValue(13.0);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name), expectedValue, 1e-12);
  }

  void test_GetPropertyAsSingleValue_Returns_Correct_SingleValue_For_Each_StatisticType()
  {
    LogManager runInfo;
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
    LogManager run;
    const std::string name = "series";
    addTestTimeSeries(run, name);

    TS_ASSERT_EQUALS(run.getPropertyAsSingleValue(name,Math::Mean), 13.0);
    TS_ASSERT_EQUALS(run.getPropertyAsSingleValue(name,Math::Mean), 13.0);
    TS_ASSERT_EQUALS(run.getPropertyAsSingleValue(name,Math::Minimum), 2.0);
    TS_ASSERT_EQUALS(run.getPropertyAsSingleValue(name,Math::Minimum), 2.0);
  }


  void test_GetPropertyAsSingleValue_Returns_Correct_Value_On_Second_Call_When_Log_Has_Been_Replaced()
  {
    LogManager runInfo;
    const std::string name = "double";
    double value(5.1);
    runInfo.addProperty(name, value);

    TS_ASSERT_EQUALS(runInfo.getPropertyAsSingleValue(name), value);

    // Replace the log with a different value
    value = 10.3;
    runInfo.addProperty(name, value, /*overwrite*/true);

    TS_ASSERT_EQUALS(runInfo.getPropertyAsSingleValue(name), value);
  }


 
  void test_clear()
  {
    // Set up a Run object with 3 properties in it (1 time series, 2 single value)
    LogManager runInfo;
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



  /** Save and load to NXS file */
  void test_nexus()
  {
    NexusTestHelper th(true);
    th.createFile("LogManagerTest.nxs");

    LogManager run1;
    addTimeSeriesEntry(run1, "double_series", 45.0);
    run1.addProperty( new PropertyWithValue<int>("int_val", 1234) );
    run1.addProperty( new PropertyWithValue<std::string>("string_val", "help_im_stuck_in_a_log_file") );
    run1.addProperty( new PropertyWithValue<double>("double_val", 5678.9) );
    addTimeSeriesEntry(run1, "phi", 12.3);
    addTimeSeriesEntry(run1, "chi", 45.6);
    addTimeSeriesEntry(run1, "omega", 78.9);
    addTimeSeriesEntry(run1, "proton_charge", 78.9);


 
    run1.saveNexus(th.file, "logs");
    th.file->openGroup("logs", "NXgroup");
    th.file->makeGroup("junk_to_ignore", "NXmaterial");
    th.file->makeGroup("more_junk_to_ignore", "NXsample");

    // ---- Now re-load the same and compare ------
    th.reopenFile();
    LogManager run2;
    run2.loadNexus(th.file, "logs");
    TS_ASSERT( run2.hasProperty("double_series") );
    TS_ASSERT( run2.hasProperty("int_val") );
    TS_ASSERT( run2.hasProperty("string_val") );
    TS_ASSERT( run2.hasProperty("double_val") );
    // This test both uses the goniometer axes AND looks up some values.



    // Reload without opening the group (for backwards-compatible reading of old files)
    LogManager run3;
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
    th.createFile("LogManagerTest.nxs");
    th.file->makeGroup("sample", "NXsample", 1);
    th.file->writeData("proton_charge", 1.234);
    th.reopenFile();
    th.file->openGroup("sample", "NXsample");
    LogManager run3;
    run3.loadNexus(th.file, "");
 
  }

};

//---------------------------------------------------------------------------------------
// Performance test
//---------------------------------------------------------------------------------------

class LogManagerTestPerformance : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LogManagerTestPerformance *createSuite() { return new LogManagerTestPerformance(); }
  static void destroySuite( LogManagerTestPerformance *suite ) { delete suite; }

  LogManagerTestPerformance() : m_testRun(), m_propName("test")
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

  LogManager m_testRun;
  std::string m_propName;
};


#endif
