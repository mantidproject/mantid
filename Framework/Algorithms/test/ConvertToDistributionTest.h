#ifndef CONVERTTODISTRIBUTIONTEST_H_
#define CONVERTTODISTRIBUTIONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/ConvertToDistribution.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using Mantid::Algorithms::ConvertToDistribution;

class ConvertToDistributionTest : public CxxTest::TestSuite {
public:
  static ConvertToDistributionTest *createSuite() {
    return new ConvertToDistributionTest();
  }
  static void destroySuite(ConvertToDistributionTest *suite) { delete suite; }

  void testName() {
    ConvertToDistribution conv;
    TS_ASSERT_EQUALS(conv.name(), "ConvertToDistribution")
  }

  void testVersion() {
    ConvertToDistribution conv;
    TS_ASSERT_EQUALS(conv.version(), 1)
  }

  void testInit() {
    ConvertToDistribution conv;
    TS_ASSERT_THROWS_NOTHING(conv.initialize())
    TS_ASSERT(conv.isInitialized())
  }

  void testExec() {
    ConvertToDistribution conv;
    conv.initialize();
    conv.setChild(true);
    auto dist = createTestWorkspace();
    TS_ASSERT_THROWS_NOTHING(conv.setProperty("Workspace", dist));
    TS_ASSERT_THROWS_NOTHING(conv.execute());
    TS_ASSERT(conv.isExecuted());
    auto output = boost::dynamic_pointer_cast<MatrixWorkspace>(dist);
    const Mantid::MantidVec &X = output->dataX(0);
    const Mantid::MantidVec &Y = output->dataY(0);
    const Mantid::MantidVec &E = output->dataE(0);
    for (size_t i = 0; i < Y.size(); ++i) {
      TS_ASSERT_EQUALS(X[i], static_cast<double>(i) / 2.0)
      TS_ASSERT_EQUALS(Y[i], 4)
      TS_ASSERT_EQUALS(E[i], sqrt(2.0) / 0.5)
    }
    TS_ASSERT(output->isDistribution())
  }

private:
  Workspace_sptr createTestWorkspace() {
    return WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 10, 0, 0.5);
  }
};

#endif /*CONVERTTODISTRIBUTIONTEST_H_*/
