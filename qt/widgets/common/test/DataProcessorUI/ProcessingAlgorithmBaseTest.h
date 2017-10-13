#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORPROCESSINGALGORITHMBASETEST_H_
#define MANTID_MANTIDWIDGETS_DATAPROCESSORPROCESSINGALGORITHMBASETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ProcessingAlgorithmBase.h"

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets::DataProcessor;

class ProcessingAlgorithmBaseTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProcessingAlgorithmBaseTest *createSuite() {
    return new ProcessingAlgorithmBaseTest();
  }
  static void destroySuite(ProcessingAlgorithmBaseTest *suite) { delete suite; }
  ProcessingAlgorithmBaseTest() { FrameworkManager::Instance(); };

  void test_MatrixWorkspace_properties() {
    // Test MatrixWorkspace properties
    ProcessingAlgorithmBase alg("MultiplyRange");
    TS_ASSERT_EQUALS(alg.getInputWsProperties(),
                     std::vector<QString>{"InputWorkspace"});
    TS_ASSERT_EQUALS(alg.getOutputWsProperties(),
                     std::vector<QString>{"OutputWorkspace"});
    TS_ASSERT_EQUALS(alg.getInputStrListProperties(), std::vector<QString>());

    // Add more algorithms to test if needed
  }

  void test_Workspace_properties() {
    // Test Workspace properties
    ProcessingAlgorithmBase alg("CompareWorkspaces");
    TS_ASSERT_EQUALS(alg.getInputWsProperties().size(), 2);
    TS_ASSERT_EQUALS(alg.getInputStrListProperties(), std::vector<QString>());

    // Add more algorithms to test if needed
  }

  void test_StrList_properties() {
    ProcessingAlgorithmBase alg("Stitch1DMany");
    TS_ASSERT_EQUALS(alg.getInputStrListProperties(),
                     std::vector<QString>{"InputWorkspaces"});
    TS_ASSERT_EQUALS(alg.getOutputWsProperties(),
                     std::vector<QString>{"OutputWorkspace"});
    TS_ASSERT_EQUALS(alg.getInputWsProperties(), std::vector<QString>());
  }
};

#endif /* MANTID_MANTIDWIDGETS_DATAPROCESSORPROCESSINGALGORITHMBASETEST_H_ */
