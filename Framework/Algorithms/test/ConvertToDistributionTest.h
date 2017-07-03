#ifndef CONVERTTODISTRIBUTIONTEST_H_
#define CONVERTTODISTRIBUTIONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/ConvertToDistribution.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"

using namespace Mantid::API;
using Mantid::Algorithms::ConvertToDistribution;

class TestConvertToDistribution : public ConvertToDistribution {
public:
  std::map<std::string, std::string> wrapValidateInputs() {
    return this->validateInputs();
  }
};

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
      TS_ASSERT_EQUALS(E[i], M_SQRT2 / 0.5)
    }
    TS_ASSERT(output->isDistribution())
  }

  /**
   * Test that the algorithm can handle a WorkspaceGroup as input without
   * crashing
   * We have to use the ADS to test WorkspaceGroups
   */
  void testValidateInputsWithWSGroup() {
    auto ws1 = createTestWorkspace();
    auto ws2 = createTestWorkspace();
    AnalysisDataService::Instance().add("workspace1", ws1);
    AnalysisDataService::Instance().add("workspace2", ws2);
    auto group = boost::make_shared<WorkspaceGroup>();
    AnalysisDataService::Instance().add("group", group);
    group->add("workspace1");
    group->add("workspace2");
    TestConvertToDistribution conv;
    conv.initialize();
    conv.setChild(true);
    TS_ASSERT_THROWS_NOTHING(conv.setPropertyValue("Workspace", "group"));
    TS_ASSERT_THROWS_NOTHING(conv.wrapValidateInputs());
    AnalysisDataService::Instance().clear();
  }

private:
  Workspace_sptr createTestWorkspace() {
    return WorkspaceCreationHelper::create2DWorkspaceBinned(1, 10, 0, 0.5);
  }
};

#endif /*CONVERTTODISTRIBUTIONTEST_H_*/
