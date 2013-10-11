#ifndef MANTID_API_PROPERTYNEXUSTEST_H_
#define MANTID_API_PROPERTYNEXUSTEST_H_

#include "MantidAPI/PropertyNexus.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include "MantidTestHelpers/NexusTestHelper.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/DateAndTime.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class PropertyNexusTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PropertyNexusTest *createSuite() { return new PropertyNexusTest(); }
  static void destroySuite( PropertyNexusTest *suite ) { delete suite; }

  /** Compare a property */
  template <typename T>
  void check_prop(Property * prop, T * other)
  {
    T * p = dynamic_cast<T*>(prop);
    TSM_ASSERT("Loaded property was not of the expected type", p);
    if (!p) return;
    TS_ASSERT_EQUALS( p->value(), other->value() );
  }

  void test_saving_then_loading()
  {
    NexusTestHelper th(true);
    th.createFile("PropertyNexusTest.nxs");
    PropertyWithValue<int> pi("int_val", 123);
    PropertyWithValue<uint32_t> pu("uint_val", 123);
    PropertyWithValue<double> pd("double_val", 456.78);
    PropertyWithValue<float> pf("float_val", float(987.56));
    PropertyWithValue<std::string> ps("string_val", "supercallifragalistic");
    PropertyWithValue<std::vector<double>> pvd("vector_double_val", std::vector<double>(2, 1.4));

    PropertyNexus::saveProperty(th.file, &pi);
    PropertyNexus::saveProperty(th.file, &pu);
    PropertyNexus::saveProperty(th.file, &pd);
    PropertyNexus::saveProperty(th.file, &pf);
    PropertyNexus::saveProperty(th.file, &ps);
    PropertyNexus::saveProperty(th.file, &pvd);

    TimeSeriesProperty<int> tspi("int_series");
    tspi.addValue( DateAndTime("2011-01-01T00:00:01"), 1234 );
    tspi.addValue( DateAndTime("2011-01-01T00:01:02"), 4567 );

    TimeSeriesProperty<double> tspd("double_series");
    tspd.addValue( DateAndTime("2011-01-01T00:00:01"), 1234.5 );
    tspd.addValue( DateAndTime("2011-01-01T00:01:02"), 4567.8 );

    TimeSeriesProperty<bool> tspb("bool_series");
    tspb.addValue( DateAndTime("2011-01-01T00:00:01"), true );
    tspb.addValue( DateAndTime("2011-01-01T00:01:02"), false );

    TimeSeriesProperty<std::string> tsps("string_series");
    tsps.addValue( DateAndTime("2011-01-01T00:00:01"), "help me i" );
    tsps.addValue( DateAndTime("2011-01-01T00:01:02"), "am stuck in a NXS file" );

    PropertyNexus::saveProperty(th.file, &tspi);
    PropertyNexus::saveProperty(th.file, &tspd);
    PropertyNexus::saveProperty(th.file, &tspb);
    PropertyNexus::saveProperty(th.file, &tsps);

    // ---- Now re-load and compare to the original ones ----------------------------
    th.reopenFile();
    Property * prop;

    prop = PropertyNexus::loadProperty(th.file, "int_val");   check_prop(prop, &pi);
    prop = PropertyNexus::loadProperty(th.file, "uint_val");   check_prop(prop, &pu);
    prop = PropertyNexus::loadProperty(th.file, "double_val");   check_prop(prop, &pd);
    prop = PropertyNexus::loadProperty(th.file, "float_val");   check_prop(prop, &pf);
    prop = PropertyNexus::loadProperty(th.file, "string_val");   check_prop(prop, &ps);
    prop = PropertyNexus::loadProperty(th.file, "vector_double_val");   check_prop(prop, &pvd);


    prop = PropertyNexus::loadProperty(th.file, "int_series");   check_prop(prop, &tspi);
    prop = PropertyNexus::loadProperty(th.file, "double_series");   check_prop(prop, &tspd);
    prop = PropertyNexus::loadProperty(th.file, "bool_series");   check_prop(prop, &tspb);
    prop = PropertyNexus::loadProperty(th.file, "string_series");   check_prop(prop, &tsps);
  }


};


#endif /* MANTID_API_PROPERTYNEXUSTEST_H_ */

