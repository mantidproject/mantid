// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_FILTERBYXVALUETEST_H_
#define MANTID_ALGORITHMS_FILTERBYXVALUETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FilterByXValue.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::FilterByXValue;

class FilterByXValueTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FilterByXValueTest *createSuite() { return new FilterByXValueTest(); }
  static void destroySuite(FilterByXValueTest *suite) { delete suite; }

  void test_validation() {
    FilterByXValue alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    // InputWorkspace has to be an EventWorkspace
    TS_ASSERT_THROWS(
        alg.setProperty("InputWorkspace",
                        WorkspaceCreationHelper::create2DWorkspace(1, 1)),
        const std::invalid_argument &);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "InputWorkspace", WorkspaceCreationHelper::createEventWorkspace()));

    // At least one of XMin & XMax must be specified
    auto errorMap = alg.validateInputs();
    TS_ASSERT_EQUALS(errorMap.size(), 2);
    TS_ASSERT_EQUALS(errorMap.begin()->first, "XMax");
    TS_ASSERT_EQUALS(errorMap.rbegin()->first, "XMin");

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMin", 10.0));
    TS_ASSERT(alg.validateInputs().empty());

    // XMax must be > XMin
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMax", 9.0));
    errorMap = alg.validateInputs();
    TS_ASSERT_EQUALS(errorMap.size(), 2);
    TS_ASSERT_EQUALS(errorMap.begin()->first, "XMax");
    TS_ASSERT_EQUALS(errorMap.rbegin()->first, "XMin");
  }

  void test_exec() {
    using Mantid::DataObjects::EventWorkspace_sptr;
    EventWorkspace_sptr inputWS =
        WorkspaceCreationHelper::createEventWorkspace2(5, 1);
    // Add the workspace to the ADS so that it gets a name (stops validation
    // complaints)
    Mantid::API::AnalysisDataService::Instance().add("inWS", inputWS);

    FilterByXValue alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "inWS"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMin", 20.5));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMax", 30.5));
    TS_ASSERT(alg.execute());

    TS_ASSERT_EQUALS(inputWS->getNumberEvents(), 110);
    TS_ASSERT_EQUALS(inputWS->getEventXMin(), 20.5);
    TS_ASSERT_EQUALS(inputWS->getEventXMax(), 30.5);
  }
};

class FilterByXValueTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FilterByXValueTestPerformance *createSuite() {
    return new FilterByXValueTestPerformance();
  }
  static void destroySuite(FilterByXValueTestPerformance *suite) {
    delete suite;
  }

  void setUp() override {
    Mantid::API::AnalysisDataService::Instance().add(
        "ToFilter", WorkspaceCreationHelper::createEventWorkspace(
                        5000, 1000, 8000, 0.0, 1.0, 3));
  }

  void tearDown() override {
    Mantid::API::AnalysisDataService::Instance().remove("ToFilter");
  }

  void test_crop_events_inplace() {
    FilterByXValue filter;
    filter.initialize();
    filter.setPropertyValue("InputWorkspace", "ToFilter");
    filter.setPropertyValue("OutputWorkspace", "ToFilter");
    filter.setProperty("XMin", 5000.0);
    filter.setProperty("XMax", 7500.0);
    TS_ASSERT(filter.execute());
  }
};

#endif /* MANTID_ALGORITHMS_FILTERBYXVALUETEST_H_ */
