#ifndef MANTID_CRYSTAL_SORTPEAKSWORKSPACETEST_H_
#define MANTID_CRYSTAL_SORTPEAKSWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <algorithm>
#include "MantidCrystal/SortPeaksWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidDataObjects/PeaksWorkspace.h"

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class SortPeaksWorkspaceTest : public CxxTest::TestSuite {
private:
  /**
   * Helper method.
   * Execute the algorithm on the given input workspace and columnName.
   * @param inWS : Input workspace to sort
   * @param columnName : Column name to sort by
   */
  void doExecute(IPeaksWorkspace_sptr inWS, const std::string &columnName,
                 const bool sortAscending = true) {
    std::string outWSName("SortPeaksWorkspaceTest_OutputWS");

    SortPeaksWorkspace alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("ColumnNameToSortBy", columnName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SortAscending", sortAscending));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    PeaksWorkspace_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>(outWSName);

    // Extract the sorted column values out into a containtainer.
    const size_t columnIndex = outWS->getColumnIndex(columnName);
    std::vector<double> potentiallySorted;
    for (size_t rowIndex = 0; rowIndex < outWS->rowCount(); ++rowIndex) {
      TableRow row = outWS->getRow(rowIndex);
      potentiallySorted.push_back(row.Double(columnIndex));
    }

    // Compare the contents of the container to determine how the column has
    // been sorted.
    if (sortAscending) {
      bool b_sortedAscending = this->isSortedAscending(potentiallySorted);
      TSM_ASSERT("The Workspace has not been sorted ascending according to the "
                 "column as expected",
                 b_sortedAscending);
    } else {
      bool b_sortedDescending = this->isSortedDescending(potentiallySorted);
      TSM_ASSERT("The Workspace has not been sorted descending according to "
                 "the column as expected",
                 b_sortedDescending);
    }

    TSM_ASSERT_DIFFERS(
        "Output and Input Workspaces should be different objects.", outWS,
        inWS);
  }

  /**
   * Helper method.
   * Determine whether a vector is sorted Ascending
   * @param potentiallySorted : Vector that might be sorted ascending.
   * @return False if not sortedAscending
   */
  template <typename T>
  bool isSortedAscending(std::vector<T> potentiallySorted) {
    return std::adjacent_find(potentiallySorted.begin(),
                              potentiallySorted.end(),
                              std::greater<T>()) == potentiallySorted.end();
  }

  /**
   * Helper method.
   * Determine whether a vector is sorted Descending
   * @param potentiallySorted : Vector that might be sorted descending.
   * @return False if not sortedAscending
   */
  template <typename T>
  bool isSortedDescending(std::vector<T> potentiallySorted) {
    return std::adjacent_find(potentiallySorted.begin(),
                              potentiallySorted.end(),
                              std::less<T>()) == potentiallySorted.end();
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SortPeaksWorkspaceTest *createSuite() {
    return new SortPeaksWorkspaceTest();
  }
  static void destroySuite(SortPeaksWorkspaceTest *suite) { delete suite; }

  void test_Init() {
    SortPeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_columnToSortBy_no_provided_throws() {
    // Name of the output workspace.
    std::string outWSName("SortPeaksWorkspaceTest_OutputWS");

    PeaksWorkspace_sptr inWS = WorkspaceCreationHelper::createPeaksWorkspace();

    SortPeaksWorkspace alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    // Note that We did not specify the "ColumnToSortBy" mandatory argument
    // before executing!
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error &);
  }

  void test_exec_with_unknown_columnToSortBy() {
    // Name of the output workspace.
    std::string outWSName("SortPeaksWorkspaceTest_OutputWS");

    PeaksWorkspace_sptr inWS = WorkspaceCreationHelper::createPeaksWorkspace();

    SortPeaksWorkspace alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ColumnNameToSortBy", "V"));
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument &);
  }

  void test_sort_by_H() {
    const std::string columnOfInterestName = "h";

    // Create a peaks workspace and add some peaks with unordered H values.
    PeaksWorkspace_sptr inWS = WorkspaceCreationHelper::createPeaksWorkspace();
    Peak peak1;
    peak1.setH(1);
    Peak peak2;
    peak2.setH(3);
    Peak peak3;
    peak3.setH(2);
    inWS->addPeak(peak1);
    inWS->addPeak(peak2);
    inWS->addPeak(peak3);

    doExecute(inWS, columnOfInterestName);
  }

  void test_sort_by_H_Descending() {
    const std::string columnOfInterestName = "h";

    // Create a peaks workspace and add some peaks with unordered H values.
    PeaksWorkspace_sptr inWS = WorkspaceCreationHelper::createPeaksWorkspace();
    Peak peak1;
    peak1.setH(0);
    Peak peak2;
    peak2.setH(2);
    Peak peak3;
    peak3.setH(1);
    inWS->addPeak(peak1);
    inWS->addPeak(peak2);
    inWS->addPeak(peak3);
    const bool sortAscending = false;

    doExecute(inWS, columnOfInterestName, sortAscending);
  }

  void test_modify_workspace_in_place() {
    PeaksWorkspace_sptr inWS = WorkspaceCreationHelper::createPeaksWorkspace();

    SortPeaksWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inWS));
    alg.setPropertyValue("OutputWorkspace", "OutName");
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", inWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ColumnNameToSortBy", "h"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    PeaksWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    TSM_ASSERT_EQUALS("Sorting should have happened in place. Output and input "
                      "workspaces should be the same.",
                      outWS, inWS);
  }
};

#endif /* MANTID_CRYSTAL_SORTPEAKSWORKSPACETEST_H_ */
