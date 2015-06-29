#ifndef MANTID_DATAHANDLING_DECORATORWORKSPACETEST_H_
#define MANTID_DATAHANDLING_DECORATORWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/TimeSeriesProperty.h"

#include <memory>

#include "MantidDataHandling/DecoratorWorkspace.h"

using Mantid::DataHandling::DecoratorWorkspace;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class DecoratorWorkspaceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DecoratorWorkspaceTest *createSuite() {
    return new DecoratorWorkspaceTest();
  }
  static void destroySuite(DecoratorWorkspaceTest *suite) { delete suite; }

  void test_constructor() {
    DecoratorWorkspace decorator;
    TSM_ASSERT_EQUALS("Always one period by default", 1, decorator.nPeriods());
  }

  void test_output_single_workspace() {
      DecoratorWorkspace decorator;
      TSM_ASSERT_EQUALS("Always one period by default", 1, decorator.nPeriods());
      TS_ASSERT_EQUALS(decorator.combinedWorkspace(), decorator.getSingleHeldWorkspace());
  }

  void test_output_multiple_workspaces(){
      DecoratorWorkspace decorator;
      std::unique_ptr<const TimeSeriesProperty<int> > periodLog(new const TimeSeriesProperty<int>("period_log"));
      decorator.setNPeriods(3, periodLog);
      WorkspaceGroup_sptr outWS = boost::dynamic_pointer_cast<WorkspaceGroup>(decorator.combinedWorkspace());
      TSM_ASSERT("Should be a WorkspaceGroup", outWS);
      TS_ASSERT_EQUALS(3, outWS->size());
  }
};

#endif /* MANTID_DATAHANDLING_DECORATORWORKSPACETEST_H_ */
