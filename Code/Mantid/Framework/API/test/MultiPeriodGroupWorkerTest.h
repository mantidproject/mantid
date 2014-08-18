#ifndef MANTID_API_MULTIPERIODGROUPWORKERTEST_H_
#define MANTID_API_MULTIPERIODGROUPWORKERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MultiPeriodGroupWorker.h"

using Mantid::API::MultiPeriodGroupWorker;
using namespace Mantid::API;

class MultiPeriodGroupWorkerTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MultiPeriodGroupWorkerTest *createSuite() { return new MultiPeriodGroupWorkerTest(); }
  static void destroySuite( MultiPeriodGroupWorkerTest *suite ) { delete suite; }


  void test_default_construction()
  {
    MultiPeriodGroupWorker worker;
    TS_ASSERT(!worker.useCustomWorkspaceProperty());
  }

  void test_regular_construction()
  {
    MultiPeriodGroupWorker worker1("InputWorkspace");
    TS_ASSERT(worker1.useCustomWorkspaceProperty());

    MultiPeriodGroupWorker worker2("InputWorkspace");
    TS_ASSERT(worker2.useCustomWorkspaceProperty());
  }


};


#endif /* MANTID_API_MULTIPERIODGROUPWORKERTEST_H_ */
