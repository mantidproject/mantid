// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_USAGESERVICETEST_H_
#define MANTID_API_USAGESERVICETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/UsageService.h"
#include <algorithm>
#include <json/json.h>

using Mantid::Kernel::UsageServiceImpl;

class TestableUsageService : public UsageServiceImpl {
public:
  TestableUsageService() : UsageServiceImpl() {}

  /// generates the message body for a startup message
  std::string generateStartupMessage() override {
    return UsageServiceImpl::generateStartupMessage();
  }
  /// generates the message body for a feature usage message
  std::string generateFeatureUsageMessage() override {
    return UsageServiceImpl::generateFeatureUsageMessage();
  }

protected:
  /// sends a report over the internet
  int sendReport(const std::string &message, const std::string &url) override {
    UNUSED_ARG(message);
    UNUSED_ARG(url);
    // do nothing
    return 200;
  }
};

class UsageServiceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static UsageServiceTest *createSuite() { return new UsageServiceTest(); }
  static void destroySuite(UsageServiceTest *suite) { delete suite; }

  void test_enabled() {
    TestableUsageService usageService;
    TS_ASSERT_EQUALS(usageService.isEnabled(), false);
    usageService.setEnabled(false);
    TS_ASSERT_EQUALS(usageService.isEnabled(), false);
    usageService.setInterval(1000);
    TS_ASSERT_EQUALS(usageService.isEnabled(), false);
    usageService.setEnabled(true);
    TS_ASSERT_EQUALS(usageService.isEnabled(), true);
    usageService.setInterval(10000);
    TS_ASSERT_EQUALS(usageService.isEnabled(), true);
    usageService.setEnabled(false);
    TS_ASSERT_EQUALS(usageService.isEnabled(), false);
  }

  void test_startupMessage() {
    TestableUsageService usageService;
    std::string name = "My testing application name";
    usageService.setApplicationName(name);
    std::string message = usageService.generateStartupMessage();

    ::Json::Reader reader;
    ::Json::Value root;
    reader.parse(message, root);
    auto members = root.getMemberNames();
    std::vector<std::string> expectedMembers{
        "ParaView", "application", "host",       "mantidSha1", "mantidVersion",
        "osArch",   "osName",      "osReadable", "osVersion",  "uid"};
    for (auto expectedMember : expectedMembers) {
      TSM_ASSERT(expectedMember + " not found",
                 std::find(members.begin(), members.end(), expectedMember) !=
                     members.end());
    }

    TS_ASSERT_EQUALS(root["application"].asString(), name);
  }

  void test_FeatureUsageMessage() {
    TestableUsageService usageService;
    usageService.setInterval(10000);
    usageService.setEnabled(true);
    usageService.registerFeatureUsage("Algorithm", "MyAlg.v1", true);
    usageService.registerFeatureUsage("Interface", "MyAlg.v1", true);
    for (size_t i = 0; i < 10000; i++) {
      usageService.registerFeatureUsage("Algorithm", "MyLoopAlg.v1", false);
    }
    usageService.registerFeatureUsage("Algorithm", "MyLoopAlg.v1", true);

    std::string message = usageService.generateFeatureUsageMessage();

    ::Json::Reader reader;
    ::Json::Value root;
    reader.parse(message, root);
    auto members = root.getMemberNames();
    std::vector<std::string> expectedMembers{"mantidVersion", "features"};
    for (auto expectedMember : expectedMembers) {
      TSM_ASSERT(expectedMember + " not found",
                 std::find(members.begin(), members.end(), expectedMember) !=
                     members.end());
    }

    auto features = root["features"];
    for (auto &feature : features) {
      std::string name = feature["name"].asString();
      std::string type = feature["type"].asString();
      bool internal = feature["internal"].asBool();
      size_t count = feature["count"].asUInt();

      bool correct = false;

      if (type == "Algorithm" && name == "MyAlg.v1" && internal == true &&
          count == 1)
        correct = true;
      if (type == "Interface" && name == "MyAlg.v1" && internal == true &&
          count == 1)
        correct = true;
      if (type == "Algorithm" && name == "MyLoopAlg.v1" && internal == false &&
          count == 10000)
        correct = true;
      if (type == "Algorithm" && name == "MyLoopAlg.v1" && internal == true &&
          count == 1)
        correct = true;
      TSM_ASSERT("Usage record was not as expected", correct)
    }
  }

  void test_Flush() {
    TestableUsageService usageService;
    usageService.setInterval(10000);
    usageService.setEnabled(true);
    for (size_t i = 0; i < 10; i++) {
      usageService.registerFeatureUsage("Algorithm", "MyLoopAlg.v1", false);
    }
    // this should empty the feature usage list
    usageService.flush();
    // so this should no be empty
    TS_ASSERT_EQUALS(usageService.generateFeatureUsageMessage(), "");
  }

  void test_Shutdown() {
    TestableUsageService usageService;
    usageService.setInterval(10000);
    usageService.setEnabled(true);
    for (size_t i = 0; i < 10; i++) {
      usageService.registerFeatureUsage("Algorithm", "MyLoopAlg.v1", false);
    }
    // this should empty the feature usage list
    usageService.shutdown();
    // so this should no be empty
    TS_ASSERT_EQUALS(usageService.generateFeatureUsageMessage(), "");
    // and it should be disabled
    TS_ASSERT_EQUALS(usageService.isEnabled(), false);
  }

  void test_setApplicationName() {
    TestableUsageService usageService;
    // test default first
    TS_ASSERT_EQUALS(usageService.getApplicationName(), "python");

    std::string name = "My testing application name";
    usageService.setApplicationName(name);
    TS_ASSERT_EQUALS(usageService.getApplicationName(), name);
  }
};

#endif /* MANTID_API_USAGESERVICETEST_H_ */