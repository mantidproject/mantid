#ifndef MANTID_ALGORITHMS_PROCESSBACKGROUNDTEST_H_
#define MANTID_ALGORITHMS_PROCESSBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ProcessBackground.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/WorkspaceFactory.h"

using Mantid::Algorithms::ProcessBackground;
using namespace Mantid;
using namespace Kernel;

class ProcessBackgroundTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProcessBackgroundTest *createSuite() { return new ProcessBackgroundTest(); }
  static void destroySuite( ProcessBackgroundTest *suite ) { delete suite; }

  /*
   * Test option delete region
   */
  void test_DeleteRegion()
  {
      // 1. Create Workspace2D
      DataObjects::Workspace2D_sptr inpws
              = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(API::WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10));
      for (size_t i = 0; i < 10; ++i)
      {
          inpws->dataX(0)[i] = double(i);
          inpws->dataY(0)[i] = double(i)*double(i);
      }
      API::AnalysisDataService::Instance().addOrReplace("Background1", inpws);

      // 2. Do the job
      ProcessBackground alg;
      TS_ASSERT_THROWS_NOTHING(alg.initialize());
      TS_ASSERT(alg.isInitialized());

      TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inpws));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "NewBackground"));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("Options", "DeleteRegion"));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("LowerBound", 4.5));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("UpperBound", 6.3));

      TS_ASSERT_THROWS_NOTHING(alg.execute());
      TS_ASSERT(alg.isExecuted());

      // 3. Check
      DataObjects::Workspace2D_sptr outws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
              (API::AnalysisDataService::Instance().retrieve("NewBackground"));
      size_t newsize = outws->dataX(0).size();

      TS_ASSERT_EQUALS(newsize, 8);

      // 4. Clean
      API::AnalysisDataService::Instance().remove("Background1");
      API::AnalysisDataService::Instance().remove("NewBackground");

      return;
  }

  /*
   * Test option "Add Region"
   */
  void test_AddRegion()
  {
      // 1. Create Workspace2D
      DataObjects::Workspace2D_sptr inpws
              = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(API::WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10));
      for (size_t i = 0; i < 10; ++i)
      {
          inpws->dataX(0)[i] = double(i);
          inpws->dataY(0)[i] = double(i)*double(i);
      }
      API::AnalysisDataService::Instance().addOrReplace("Background2", inpws);

      DataObjects::Workspace2D_sptr refws
              = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(API::WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10));
      for (size_t i = 0; i < 10; ++i)
      {
          refws->dataX(0)[i] = double(i)*0.3+1.01;
          refws->dataY(0)[i] = double(i)*double(i);
      }
      API::AnalysisDataService::Instance().addOrReplace("RefBackground", refws);

      // 2. Do the job
      ProcessBackground alg;
      TS_ASSERT_THROWS_NOTHING(alg.initialize());
      TS_ASSERT(alg.isInitialized());

      TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inpws));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "NewBackground"));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("ReferenceWorkspace", refws));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("Options", "AddRegion"));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("LowerBound", 1.001));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("UpperBound", 1.99));

      TS_ASSERT_THROWS_NOTHING(alg.execute());
      TS_ASSERT(alg.isExecuted());

      // 3. Check
      DataObjects::Workspace2D_sptr outws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
              (API::AnalysisDataService::Instance().retrieve("NewBackground"));
      size_t newsize = outws->dataX(0).size();

      TS_ASSERT_EQUALS(newsize, 14);

      /*
      for (size_t i = 0; i < newsize; ++i)
          std::cout << i << " :  " << outws->dataX(0)[i] << std::endl;
      */

      // 4. Clean
      API::AnalysisDataService::Instance().remove("Background2");
      API::AnalysisDataService::Instance().remove("NewBackground");

      return;
  }


};


#endif /* MANTID_ALGORITHMS_PROCESSBACKGROUNDTEST_H_ */
