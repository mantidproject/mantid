// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/NexusTestHelper.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/PropertyNexus.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::Types::Core::DateAndTime;

class PropertyNexusTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PropertyNexusTest *createSuite() { return new PropertyNexusTest(); }
  static void destroySuite(PropertyNexusTest *suite) { delete suite; }

  /** Compare a property */
  template <typename T> void check_prop(Property *prop, T *other) {
    T *p = dynamic_cast<T *>(prop);
    TSM_ASSERT("Loaded property was not of the expected type", p);
    if (!p)
      return;
    TS_ASSERT_EQUALS(p->value(), other->value());
    if (!(other->units().empty())) {
      // for some reason empty units read in as a single space
      TS_ASSERT_EQUALS(p->units(), other->units());
    }
  }

  void test_saving_then_loading() {
    NexusTestHelper th(true);
    th.createFile("PropertyNexusTest.nxs");
    PropertyWithValue<int> pi("int_val", 123);
    PropertyWithValue<uint32_t> pu("uint_val", 123);
    PropertyWithValue<double> pd("double_val", 456.78);
    pd.setUnits("meter");
    PropertyWithValue<float> pf("float_val", float(987.56));
    PropertyWithValue<std::string> ps("string_val", "supercallifragalistic");
    PropertyWithValue<std::vector<double>> pvd("vector_double_val", std::vector<double>(2, 1.4));

    pi.saveProperty(th.file.get());
    pu.saveProperty(th.file.get());
    pd.saveProperty(th.file.get());
    pf.saveProperty(th.file.get());
    ps.saveProperty(th.file.get());
    pvd.saveProperty(th.file.get());

    TimeSeriesProperty<int> tspi("int_series");
    tspi.addValue(DateAndTime("2011-01-01T00:00:01"), 1234);
    tspi.addValue(DateAndTime("2011-01-01T00:01:02"), 4567);

    TimeSeriesProperty<double> tspd("double_series");
    tspd.addValue(DateAndTime("2011-01-01T00:00:01"), 1234.5);
    tspd.addValue(DateAndTime("2011-01-01T00:01:02"), 4567.8);

    TimeSeriesProperty<bool> tspb("bool_series");
    tspb.addValue(DateAndTime("2011-01-01T00:00:01"), true);
    tspb.addValue(DateAndTime("2011-01-01T00:01:02"), false);

    TimeSeriesProperty<std::string> tsps("string_series");
    tsps.addValue(DateAndTime("2011-01-01T00:00:01"), "help me i");
    tsps.addValue(DateAndTime("2011-01-01T00:01:02"), "am stuck in a NXS file");

    tspi.saveProperty(th.file.get());
    tspd.saveProperty(th.file.get());
    tspb.saveProperty(th.file.get());
    tsps.saveProperty(th.file.get());

    // ---- Now re-load and compare to the original ones
    // ----------------------------
    th.reopenFile();

    check_prop(PropertyNexus::loadProperty(th.file.get(), "int_val").get(), &pi);
    check_prop(PropertyNexus::loadProperty(th.file.get(), "uint_val").get(), &pu);
    check_prop(PropertyNexus::loadProperty(th.file.get(), "double_val").get(), &pd);
    check_prop(PropertyNexus::loadProperty(th.file.get(), "float_val").get(), &pf);
    check_prop(PropertyNexus::loadProperty(th.file.get(), "string_val").get(), &ps);
    check_prop(PropertyNexus::loadProperty(th.file.get(), "vector_double_val").get(), &pvd);

    check_prop(PropertyNexus::loadProperty(th.file.get(), "int_series").get(), &tspi);
    check_prop(PropertyNexus::loadProperty(th.file.get(), "double_series").get(), &tspd);
    check_prop(PropertyNexus::loadProperty(th.file.get(), "bool_series").get(), &tspb);
    check_prop(PropertyNexus::loadProperty(th.file.get(), "string_series").get(), &tsps);
  }
};
