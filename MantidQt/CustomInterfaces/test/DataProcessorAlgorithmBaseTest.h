#ifndef MANTID_CUSTOMINTERFACES_DATAPROCESSORALGORITHMBASETEST_H_
#define MANTID_CUSTOMINTERFACES_DATAPROCESSORALGORITHMBASETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorAlgorithmBase.h"

using namespace Mantid::API;

using MantidQt::CustomInterfaces::DataProcessorAlgorithmBase;

class DataProcessorAlgorithmBaseTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataProcessorAlgorithmBaseTest *createSuite() {
    return new DataProcessorAlgorithmBaseTest();
  }
  static void destroySuite(DataProcessorAlgorithmBaseTest *suite) {
    delete suite;
  }
  DataProcessorAlgorithmBaseTest() { FrameworkManager::Instance(); };

  void test_MatrixWorkspace_properties() {
    // Test MatrixWorkspace properties
    DataProcessorAlgorithmBase alg("MultiplyRange");
    TS_ASSERT_EQUALS(alg.getInputWsProperties(),
                     std::vector<std::string>{"InputWorkspace"});
    TS_ASSERT_EQUALS(alg.getOutputWsProperties(),
                     std::vector<std::string>{"OutputWorkspace"});
    TS_ASSERT_EQUALS(alg.getInputStrListProperties(),
                     std::vector<std::string>());

    // Add more algorithms to test if needed
  }

  void test_Workspace_properties() {
    // Test Workspace properties
    DataProcessorAlgorithmBase alg("CompareWorkspaces");
    TS_ASSERT_EQUALS(alg.getInputWsProperties().size(), 2);
    TS_ASSERT_EQUALS(alg.getInputStrListProperties(),
                     std::vector<std::string>());

    // Add more algorithms to test if needed
  }

  void test_StrList_properties() {
    DataProcessorAlgorithmBase alg("Stitch1DMany");
    TS_ASSERT_EQUALS(alg.getInputStrListProperties(),
                     std::vector<std::string>{"InputWorkspaces"});
    TS_ASSERT_EQUALS(alg.getOutputWsProperties(),
                     std::vector<std::string>{"OutputWorkspace"});
    TS_ASSERT_EQUALS(alg.getInputWsProperties(), std::vector<std::string>());
  }
};

#endif /* MANTID_CUSTOMINTERFACES_DATAPROCESSORALGORITHMBASETEST_H_ */