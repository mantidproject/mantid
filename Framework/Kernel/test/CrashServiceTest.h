#ifndef MANTID_API_CRASHSERVICETEST_H_
#define MANTID_API_CRASHSERVICETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/CrashService.h"
#include <algorithm>
#include <json/json.h>

using Mantid::Kernel::CrashServiceImpl;

class TestableCrashService : public CrashServiceImpl {
public:
  //TestableCrashService() : CrashServiceImpl() {}
  using CrashServiceImpl::CrashServiceImpl;

  /// generates the message body for a crash message
  std::string generateCrashMessage() override {
    return CrashServiceImpl::generateCrashMessage();
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

class CrashServiceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CrashServiceTest *createSuite() { return new CrashServiceTest(); }
  static void destroySuite(CrashServiceTest *suite) { delete suite; }

  void test_crashMessage() {
    std::string name = "My testing application name";
    TestableCrashService crashService(name);
    std::string message = crashService.generateCrashMessage();

    ::Json::Reader reader;
    ::Json::Value root;
    reader.parse(message, root);
    auto members = root.getMemberNames();
    std::vector<std::string> expectedMembers{
        "ParaView", "application", "host",       "mantidSha1", "mantidVersion",
        "osArch",   "osName",      "osReadable", "osVersion",  "uid", "facility"};
    for (auto expectedMember : expectedMembers) {
      TSM_ASSERT(expectedMember + " not found",
                 std::find(members.begin(), members.end(), expectedMember) !=
                     members.end());
    }

    TS_ASSERT_EQUALS(root["application"].asString(), name);
  }
};

#endif /* MANTID_API_USAGESERVICETEST_H_ */