#ifndef MANTID_GEOMETRY_XMLLOGFILETEST_H_
#define MANTID_GEOMETRY_XMLLOGFILETEST_H_

#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidGeometry/Instrument/XMLInstrumentParameter.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class XMLInstrumentParameterTest : public CxxTest::TestSuite
{
private:

  typedef boost::shared_ptr<XMLInstrumentParameter>  XMLInstrumentParameter_sptr;

  /**
  Construction logic for the XMLInstrumentParameter type isn't great, so this method acts a helper to keep the test methods cleaner.
  */
  XMLInstrumentParameter_sptr make_logfile_object(const std::string& filterBy)
  {
    const std::string logfileID = "1";
    const std::string value;
    const boost::shared_ptr<Kernel::Interpolation> interpolation = boost::make_shared<Interpolation>();
    const std::string formula;
    const std::string formulaUnit; 
    const std::string resultUnit; 
    const std::string paramName;
    const std::string type; 
    const std::string tie;
    const std::vector<std::string> constraint; 
    std::string penaltyFactor;
    const std::string fitFunc; 
    const std::string eq; 
    const Geometry::IComponent* comp = NULL; 
    double angleConvertConst = 0.0;

    return boost::shared_ptr<XMLInstrumentParameter>(new XMLInstrumentParameter(logfileID, value, interpolation, formula, formulaUnit, resultUnit, paramName, type, tie, constraint, penaltyFactor, fitFunc, filterBy, eq, comp, angleConvertConst));
  }

public:

  void test_throws_with_unknown_flag()
  {
    TimeSeriesProperty<double> series("doubleProperty"); 
    series.addValue("2000-11-30T01:01:01", 1);

    const std::string made_up_flag = "mode"; //We do not support mode statistics filtering.
    XMLInstrumentParameter_sptr logFile =  make_logfile_object(made_up_flag);

    TSM_ASSERT_THROWS("Unknown flag should cause failure", logFile->createParamValue(&series), Kernel::Exception::InstrumentDefinitionError)
  }

  void test_filter_by_first_value()
  {
    TimeSeriesProperty<double> series("doubleProperty"); 
 
    const double expectedFilteredValue = 1;
    series.addValue("2000-11-30T01:01:01", expectedFilteredValue);
    series.addValue("2000-11-30T01:01:02", 2);

    XMLInstrumentParameter_sptr logFile =  make_logfile_object("first_value");
    const double actualFilteredValue = logFile->createParamValue(&series);
    TSM_ASSERT_EQUALS("Filtering by First Value is not performed correctly", expectedFilteredValue, actualFilteredValue);
  }

  void test_filter_by_last_value()
  {
    TimeSeriesProperty<double> series("doubleProperty"); 
 
    const double expectedFilteredValue = 1;
    series.addValue("2000-11-30T01:01:01", 0);
    series.addValue("2000-11-30T01:01:02", expectedFilteredValue);

    XMLInstrumentParameter_sptr logFile =  make_logfile_object("last_value");
    const double actualFilteredValue = logFile->createParamValue(&series);
    TSM_ASSERT_EQUALS("Filtering by Last Value is not performed correctly", expectedFilteredValue, actualFilteredValue);
  }

  void test_filter_by_maximum_value()
  {
    TimeSeriesProperty<double> series("doubleProperty"); 
 
    const double expectedFilteredValue = 1;
    series.addValue("2000-11-30T01:01:01", 0.1);
    series.addValue("2000-11-30T01:01:02", expectedFilteredValue); // maximum. 1 > 0.9 > 0.1
    series.addValue("2000-11-30T01:01:03", 0.9);

    XMLInstrumentParameter_sptr logFile =  make_logfile_object("maximum");
    const double actualFilteredValue = logFile->createParamValue(&series);
    TSM_ASSERT_EQUALS("Filtering by Maximum is not performed correctly", expectedFilteredValue, actualFilteredValue);
  }

  void test_filter_by_minimum_value()
  {
    TimeSeriesProperty<double> series("doubleProperty"); 
 
    const double expectedFilteredValue = 1;
    series.addValue("2000-11-30T01:01:01", 3);
    series.addValue("2000-11-30T01:01:02", expectedFilteredValue); // minimum. 1 < 3 < 4
    series.addValue("2000-11-30T01:01:03", 4);

    XMLInstrumentParameter_sptr logFile =  make_logfile_object("minimum");
    const double actualFilteredValue = logFile->createParamValue(&series);
    TSM_ASSERT_EQUALS("Filtering by Minimum is not performed correctly", expectedFilteredValue, actualFilteredValue);
  }

  void test_filter_by_mean_value()
  {
    TimeSeriesProperty<double> series("doubleProperty"); 
 
    const double expectedFilteredValue = 1;
    series.addValue("2000-11-30T01:01:01", 0);
    series.addValue("2000-11-30T01:01:02", expectedFilteredValue); // time series mean = value at T = (T1 + T2 + T3) / 3
    series.addValue("2000-11-30T01:01:03", 2);


    XMLInstrumentParameter_sptr logFile =  make_logfile_object("mean");
    const double actualFilteredValue = logFile->createParamValue(&series);
    TSM_ASSERT_EQUALS("Filtering by Mean is not performed correctly", expectedFilteredValue, actualFilteredValue);
  }

  void test_filter_by_median_value()
  {
    TimeSeriesProperty<double> series("doubleProperty"); 
 
    const double expectedFilteredValue = 2;
    series.addValue("2000-11-30T01:01:01", 0);
    series.addValue("2000-11-30T01:01:02", 1);
    series.addValue("2000-11-30T01:01:03", expectedFilteredValue); // Median time.
    series.addValue("2000-11-30T01:01:04", 4);
    series.addValue("2000-11-30T01:02:00", 5); 

    XMLInstrumentParameter_sptr logFile =  make_logfile_object("median");
    const double actualFilteredValue = logFile->createParamValue(&series);
    TSM_ASSERT_EQUALS("Filtering by Median is not performed correctly", expectedFilteredValue, actualFilteredValue);
  }

  // This functionality will soon be legacy, since filtering by nth-position is not a good idea.
  void test_filter_by_nth_position()
  {
    TimeSeriesProperty<double> series("doubleProperty"); 
 
    const double expectedFilteredValue = 1;
    series.addValue("2000-11-30T01:01:01", 0);
    series.addValue("2000-11-30T01:01:02", expectedFilteredValue);
    series.addValue("2000-11-30T01:01:03", 2); 
    series.addValue("2000-11-30T01:01:04", 3);

    XMLInstrumentParameter_sptr logFile =  make_logfile_object("position 1");
    const double actualFilteredValue = logFile->createParamValue(&series);
    TSM_ASSERT_EQUALS("Filtering by Nth position is not performed correctly", expectedFilteredValue, actualFilteredValue);
  }



};


#endif /* MANTID_GEOMETRY_XMLLOGFILETEST_H_ */

