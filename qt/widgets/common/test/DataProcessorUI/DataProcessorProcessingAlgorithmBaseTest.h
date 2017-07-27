#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORPROCESSINGALGORITHMBASETEST_H_
#define MANTID_MANTIDWIDGETS_DATAPROCESSORPROCESSINGALGORITHMBASETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorProcessingAlgorithmBase.h"

using namespace Mantid::API;

using MantidQt::MantidWidgets::DataProcessorProcessingAlgorithmBase;

class DataProcessorProcessingAlgorithmBaseTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataProcessorProcessingAlgorithmBaseTest *createSuite() {
    return new DataProcessorProcessingAlgorithmBaseTest();
  }
  static void destroySuite(DataProcessorProcessingAlgorithmBaseTest *suite) {
    delete suite;
  }
  DataProcessorProcessingAlgorithmBaseTest() { FrameworkManager::Instance(); };

  void test_MatrixWorkspace_properties() {
    // Test MatrixWorkspace properties
    DataProcessorProcessingAlgorithmBase alg("MultiplyRange");
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
    DataProcessorProcessingAlgorithmBase alg("CompareWorkspaces");
    TS_ASSERT_EQUALS(alg.getInputWsProperties().size(), 2);
    TS_ASSERT_EQUALS(alg.getInputStrListProperties(),
                     std::vector<std::string>());

    // Add more algorithms to test if needed
  }

  void test_StrList_properties() {
    DataProcessorProcessingAlgorithmBase alg("Stitch1DMany");
    TS_ASSERT_EQUALS(alg.getInputStrListProperties(),
                     std::vector<std::string>{"InputWorkspaces"});
    TS_ASSERT_EQUALS(alg.getOutputWsProperties(),
                     std::vector<std::string>{"OutputWorkspace"});
    TS_ASSERT_EQUALS(alg.getInputWsProperties(), std::vector<std::string>());
  }
};

#endif /* MANTID_MANTIDWIDGETS_DATAPROCESSORPROCESSINGALGORITHMBASETEST_H_ */
