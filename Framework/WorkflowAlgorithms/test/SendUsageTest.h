#ifndef MANTID_WORKFLOWALGORITHMS_SENDUSAGETEST_H_
#define MANTID_WORKFLOWALGORITHMS_SENDUSAGETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/ConfigService.h"
#include "MantidWorkflowAlgorithms/SendUsage.h"

using Mantid::WorkflowAlgorithms::SendUsage;
using namespace Mantid::API;

class SendUsageTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SendUsageTest *createSuite() { return new SendUsageTest(); }
  static void destroySuite(SendUsageTest *suite) { delete suite; }

  void test_Init() {
    SendUsage alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // turn of actually sending
    const std::string SEND_USAGE_CONFIG_KEY("usagereports.enabled");
    Mantid::Kernel::ConfigService::Instance().setString(SEND_USAGE_CONFIG_KEY,
                                                        "0");

    // run the algorithm
    SendUsage alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // check the results
    const std::string json = alg.getPropertyValue("Json");
    TS_ASSERT(!json.empty());
    const std::string status = alg.getPropertyValue("HtmlCode");
    TS_ASSERT_EQUALS(status, "-1"); // not actually run
  }
};

#endif /* MANTID_WORKFLOWALGORITHMS_SENDUSAGETEST_H_ */
