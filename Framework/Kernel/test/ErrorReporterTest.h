#ifndef MANTID_API_ERRORSERVICETEST_H_
#define MANTID_API_ERRORSERVICETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/ErrorReporter.h"
#include <algorithm>
#include <json/json.h>

using Mantid::Kernel::ErrorReporter;

class TestableErrorReporter : public ErrorReporter {
public:
  using ErrorReporter::ErrorReporter;

  /// generates the message body for a error message
  std::string generateErrorMessage() override {
    return ErrorReporter::generateErrorMessage();
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

class ErrorReporterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ErrorReporterTest *createSuite() { return new ErrorReporterTest(); }
  static void destroySuite(ErrorReporterTest *suite) { delete suite; }

  void test_errorMessage() {
    std::string name = "My testing application name";
    Mantid::Types::Core::time_duration upTime(5, 0, 7, 0);
    TestableErrorReporter errorService(name, upTime, "0", false);
    std::string message = errorService.generateErrorMessage();

    ::Json::Reader reader;
    ::Json::Value root;
    reader.parse(message, root);
    auto members = root.getMemberNames();
    std::vector<std::string> expectedMembers{
        "ParaView", "application", "host",       "mantidSha1", "mantidVersion",
        "osArch",   "osName",      "osReadable", "osVersion",  "uid",
        "facility", "upTime",      "exitCode"};
    for (auto expectedMember : expectedMembers) {
      TSM_ASSERT(expectedMember + " not found",
                 std::find(members.begin(), members.end(), expectedMember) !=
                     members.end());
    }

    TS_ASSERT_EQUALS(root["application"].asString(), name);
    TS_ASSERT_EQUALS(root["upTime"].asString(), to_simple_string(upTime));
    TS_ASSERT_EQUALS(root["exitCode"].asString(), "0");
  }

  void test_errorMessageWithShare() {
    std::string name = "My testing application name";
    Mantid::Types::Core::time_duration upTime(5, 0, 7, 0);
    TestableErrorReporter errorService(name, upTime, "0", true, "name",
                                       "email");
    std::string message = errorService.generateErrorMessage();

    ::Json::Reader reader;
    ::Json::Value root;
    reader.parse(message, root);
    auto members = root.getMemberNames();
    std::vector<std::string> expectedMembers{
        "ParaView", "application", "host",       "mantidSha1", "mantidVersion",
        "osArch",   "osName",      "osReadable", "osVersion",  "uid",
        "facility", "upTime",      "exitCode",   "textBox",  "name",       "email"};
    for (auto expectedMember : expectedMembers) {
      TSM_ASSERT(expectedMember + " not found",
                 std::find(members.begin(), members.end(), expectedMember) !=
                     members.end());
    }

    TS_ASSERT_EQUALS(root["application"].asString(), name);
    TS_ASSERT_EQUALS(root["upTime"].asString(), to_simple_string(upTime));
    TS_ASSERT_EQUALS(root["exitCode"].asString(), "0");
    TS_ASSERT_EQUALS(root["name"].asString(), "name");
    TS_ASSERT_EQUALS(root["email"].asString(), "email");
  }
};

#endif /* MANTID_API_USAGESERVICETEST_H_ */
