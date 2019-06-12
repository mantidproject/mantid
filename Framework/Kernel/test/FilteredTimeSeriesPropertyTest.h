// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_FILTEREDTIMESERIESPROPERTYTEST_H_
#define MANTID_KERNEL_FILTEREDTIMESERIESPROPERTYTEST_H_

#include "MantidKernel/FilteredTimeSeriesProperty.h"
#include <cxxtest/TestSuite.h>

using Mantid::Kernel::FilteredTimeSeriesProperty;

class FilteredTimeSeriesPropertyTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FilteredTimeSeriesPropertyTest *createSuite() {
    return new FilteredTimeSeriesPropertyTest();
  }
  static void destroySuite(FilteredTimeSeriesPropertyTest *suite) {
    delete suite;
  }

  void test_FilteredProperty_Has_Same_Name_As_Original() {
    using Mantid::Kernel::TimeSeriesProperty;
    const std::string name = "seriesName";
    auto source = createTestSeries(name);
    auto filter = createTestFilter();

    auto filtered = std::make_unique<FilteredTimeSeriesProperty<double>>(
        std::move(source), filter);
    TS_ASSERT_EQUALS(filtered->name(), name);
  }

  void
  test_Transferring_Ownership_Makes_Unfiltered_Property_Return_The_Original() {
    constexpr bool transferOwnership(true);
    doOwnershipTest(transferOwnership);
  }

  void
  test_Retaining_Ownership_With_Caller_Makes_Unfiltered_Property_A_Clone() {
    constexpr bool transferOwnership(false);
    doOwnershipTest(transferOwnership);
  }

  void
  test_Construction_Yields_A_Filtered_Property_When_Accessing_Through_The_Filtered_Object() {
    auto source = createTestSeries("name");
    auto filter = createTestFilter();

    auto filtered = std::make_unique<FilteredTimeSeriesProperty<double>>(
        std::move(source), filter);

    TS_ASSERT_EQUALS(filtered->size(), 2);
    TS_ASSERT_EQUALS(filtered->nthInterval(0).begin_str(),
                     "2007-Nov-30 16:17:25");
    TS_ASSERT_EQUALS(filtered->nthInterval(0).end_str(),
                     "2007-Nov-30 16:17:30");
    TS_ASSERT_EQUALS(filtered->nthValue(0), 3);

    TS_ASSERT_EQUALS(filtered->nthInterval(1).begin_str(),
                     "2007-Nov-30 16:17:30");
    TS_ASSERT_EQUALS(filtered->nthInterval(1).end_str(),
                     "2007-Nov-30 16:17:39");
    TS_ASSERT_EQUALS(filtered->nthValue(1), 4);
  }

private:
  void doOwnershipTest(const bool transferOwnership) {
    auto source = createTestSeries("name");
    auto filter = createTestFilter();

    std::unique_ptr<FilteredTimeSeriesProperty<double>> filtered = nullptr;

    // Pointer comparison
    if (transferOwnership) {
      auto copy = std::unique_ptr<Mantid::Kernel::TimeSeriesProperty<double>>(
          source->clone());
      TS_ASSERT_THROWS_NOTHING(
          filtered = std::make_unique<FilteredTimeSeriesProperty<double>>(
              std::move(source), filter));
      source = std::move(copy);
    } else {
      TS_ASSERT_THROWS_NOTHING(
          filtered = std::make_unique<FilteredTimeSeriesProperty<double>>(
              source.get(), filter));
      TS_ASSERT_DIFFERS(source.get(), filtered->unfiltered());
    }

    // Object equality
    TS_ASSERT_EQUALS(*source,
                     *(filtered->unfiltered())); // Objects are considered equal
  }

  /// Create the test source property
  std::unique_ptr<Mantid::Kernel::TimeSeriesProperty<double>>
  createTestSeries(const std::string &name) {
    auto source =
        std::make_unique<Mantid::Kernel::TimeSeriesProperty<double>>(name);
    source->addValue("2007-11-30T16:17:00", 1);
    source->addValue("2007-11-30T16:17:10", 2);
    source->addValue("2007-11-30T16:17:20", 3);
    source->addValue("2007-11-30T16:17:30", 4);
    source->addValue("2007-11-30T16:17:40", 5);
    return source;
  }

  /// Create test filter
  Mantid::Kernel::TimeSeriesProperty<bool> createTestFilter() {
    auto filter = Mantid::Kernel::TimeSeriesProperty<bool>("filter");
    filter.addValue("2007-11-30T16:16:50", false);
    filter.addValue("2007-11-30T16:17:25", true);
    filter.addValue("2007-11-30T16:17:39", false);
    return filter;
  }
};

#endif /* MANTID_KERNEL_FILTEREDTIMESERIESPROPERTYTEST_H_ */
