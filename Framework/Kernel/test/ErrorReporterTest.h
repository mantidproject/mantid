// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
    const std::string name = "My testing application name";
    const Mantid::Types::Core::time_duration upTime(5, 0, 7, 0);
    TestableErrorReporter errorService(name, upTime, "0", false);
    const std::string message = errorService.generateErrorMessage();

    ::Json::Reader reader;
    ::Json::Value root;
    reader.parse(message, root);
    auto members = root.getMemberNames();
    const std::vector<std::string> expectedMembers{
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
    const std::string name = "My testing application name";
    const Mantid::Types::Core::time_duration upTime(5, 0, 7, 0);
    TestableErrorReporter errorService(name, upTime, "0", true, "name", "email",
                                       "textBox");
    const std::string message = errorService.generateErrorMessage();

    ::Json::Reader reader;
    ::Json::Value root;
    reader.parse(message, root);
    auto members = root.getMemberNames();
    const std::vector<std::string> expectedMembers{
        "ParaView",      "application", "host",     "mantidSha1",
        "mantidVersion", "osArch",      "osName",   "osReadable",
        "osVersion",     "uid",         "facility", "upTime",
        "exitCode",      "textBox",     "name",     "email"};
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
    TS_ASSERT_EQUALS(root["textBox"].asString(), "textBox");
  }

  void test_errorMessageWithShareAndRecoveryFileHash() {
    const std::string name = "My testing application name";
    const Mantid::Types::Core::time_duration upTime(5, 0, 7, 0);
    TestableErrorReporter errorService(name, upTime, "0", true, "name", "email",
                                       "textBox", "fileHash");
    const std::string message = errorService.generateErrorMessage();

    ::Json::Reader reader;
    ::Json::Value root;
    reader.parse(message, root);
    auto members = root.getMemberNames();
    const std::vector<std::string> expectedMembers{
        "ParaView", "application", "host",       "mantidSha1", "mantidVersion",
        "osArch",   "osName",      "osReadable", "osVersion",  "uid",
        "facility", "upTime",      "exitCode",   "textBox",    "name",
        "email",    "fileHash"};
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
    TS_ASSERT_EQUALS(root["textBox"].asString(), "textBox");
    TS_ASSERT_EQUALS(root["fileHash"].asString(), "fileHash");
  }

  void test_errorMessageWithNoShareAndRecoveryFileHash() {
    const std::string name = "My testing application name";
    const Mantid::Types::Core::time_duration upTime(5, 0, 7, 0);
    TestableErrorReporter errorService(name, upTime, "0", false, "name",
                                       "email", "textBox", "fileHash");
    const std::string message = errorService.generateErrorMessage();

    ::Json::Reader reader;
    ::Json::Value root;
    reader.parse(message, root);
    auto members = root.getMemberNames();
    const std::vector<std::string> expectedMembers{
        "ParaView", "application", "host",       "mantidSha1", "mantidVersion",
        "osArch",   "osName",      "osReadable", "osVersion",  "uid",
        "facility", "upTime",      "exitCode",   "textBox",    "name",
        "email",    "fileHash"};
    for (auto expectedMember : expectedMembers) {
      TSM_ASSERT(expectedMember + " not found",
                 std::find(members.begin(), members.end(), expectedMember) !=
                     members.end());
    }

    TS_ASSERT_EQUALS(root["application"].asString(), name);
    TS_ASSERT_EQUALS(root["upTime"].asString(), to_simple_string(upTime));
    TS_ASSERT_EQUALS(root["exitCode"].asString(), "0");
    TS_ASSERT_EQUALS(root["name"].asString(), "");
    TS_ASSERT_EQUALS(root["email"].asString(), "");
    TS_ASSERT_EQUALS(root["textBox"].asString(), "");
    TS_ASSERT_EQUALS(root["fileHash"].asString(), "");
  }
};

#endif /* MANTID_API_USAGESERVICETEST_H_ */
