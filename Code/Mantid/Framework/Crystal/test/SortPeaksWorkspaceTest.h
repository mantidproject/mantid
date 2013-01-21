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

class SortPeaksWorkspaceTest : public CxxTest::TestSuite
{
private:

  /**
   * Helper method.
   * Execute the algorithm on the given input workspace and columnName.
   * @param inWS : Input workspace to sort
   * @param columnName : Column name to sort by
   * @return Output workspace from algorithm execution
   */
  PeaksWorkspace_sptr doExecute(IPeaksWorkspace_sptr inWS, const std::string& columnName)
  {
    std::string outWSName("SortPeaksWorkspaceTest_OutputWS");

    SortPeaksWorkspace alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT( alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", inWS));
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("ColumnNameToSortBy", columnName));
    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT(alg.isExecuted());

    return AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>(outWSName);
  }

  /**
   * Helper method.
   * Determine whether a vector is sorted Ascending
   * @param potentiallySorted : Vector that might be sorted ascending.
   * @return False if not sortedAscending
   */
  template<typename T>
  bool isSortedAscending(std::vector<T> potentiallySorted)
  {
    return std::adjacent_find(potentiallySorted.begin(), potentiallySorted.end(), std::greater<T>()) == potentiallySorted.end();
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SortPeaksWorkspaceTest *createSuite() { return new SortPeaksWorkspaceTest(); }
  static void destroySuite( SortPeaksWorkspaceTest *suite ) { delete suite; }


  void test_Init()
  {
    SortPeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_columnToSortBy_no_provided_throws()
  {
    // Name of the output workspace.
    std::string outWSName("SortPeaksWorkspaceTest_OutputWS");

    PeaksWorkspace_sptr inWS = WorkspaceCreationHelper::createPeaksWorkspace();

    SortPeaksWorkspace alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT( alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", inWS));
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName));
    // Note that We did not specify the "ColumnToSortBy" mandatory argument before executing!
    TS_ASSERT_THROWS( alg.execute(), std::runtime_error&);
  }
  
  void test_exec_with_unknown_columnToSortBy()
  {
    // Name of the output workspace.
    std::string outWSName("SortPeaksWorkspaceTest_OutputWS");

    PeaksWorkspace_sptr inWS = WorkspaceCreationHelper::createPeaksWorkspace();
  
    SortPeaksWorkspace alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", inWS) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("ColumnNameToSortBy", "V"));
    TS_ASSERT_THROWS( alg.execute(), std::invalid_argument& );
  }

  void test_sort_by_H()
  {
    const std::string columnOfInterestName = "h";
    PeaksWorkspace_sptr inWS = WorkspaceCreationHelper::createPeaksWorkspace();
    PeaksWorkspace_sptr outWS = doExecute(inWS, columnOfInterestName);

    const size_t columnIndex = outWS->getColumnIndex(columnOfInterestName);
    std::vector<double> potentiallySorted;
    for(size_t rowIndex = 0; rowIndex < outWS->rowCount(); ++rowIndex)
    {
      TableRow row = outWS->getRow(rowIndex);
      potentiallySorted.push_back( row.Double(columnIndex) );
    }
    bool b_sortedAscending = this->isSortedAscending(potentiallySorted);
    TSM_ASSERT("The Workspace has not been sorted correctly", b_sortedAscending);

  }

  void tryToSortEverthing()
  {

  }

};


#endif /* MANTID_CRYSTAL_SORTPEAKSWORKSPACETEST_H_ */
