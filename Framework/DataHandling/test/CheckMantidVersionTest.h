#ifndef MANTID_DATAHANDLING_CHECKMANTIDVERSIONTEST_H_
#define MANTID_DATAHANDLING_CHECKMANTIDVERSIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/CheckMantidVersion.h"

using Mantid::DataHandling::CheckMantidVersion;
using namespace Mantid::API;

namespace {
/**
 * Mock out the version calls of this algorithm
 */
class MockedCheckMantidVersion : public CheckMantidVersion {
public:
  MockedCheckMantidVersion(std::string currentVersion,
                           std::string gitHubVersion)
      : CheckMantidVersion(), CurrentVersion(currentVersion),
        GitHubVersion(gitHubVersion) {}

  std::string CurrentVersion;
  std::string GitHubVersion;

private:
  std::string getVersionsFromGitHub(const std::string &url) override {
    // the initial assignment of the value to url is just to suppress a compiler
    // warning
    std::string outputString(url);
    outputString =
        "{\n"
        "  \"url\": "
        "\"https://api.github.com/repos/mantidproject/mantid/releases/"
        "1308203\",\n"
        "  \"assets_url\": "
        "\"https://api.github.com/repos/mantidproject/mantid/releases/1308203/"
        "assets\",\n"
        "  \"upload_url\": "
        "\"https://uploads.github.com/repos/mantidproject/mantid/releases/"
        "1308203/assets{?name}\",\n"
        "  \"html_url\": "
        "\"https://github.com/mantidproject/mantid/releases/tag/v3.4.0\",\n"
        "  \"id\": 1308203,\n"
        "  \"tag_name\": \"" +
        GitHubVersion +
        "\",\n"
        "  \"target_commitish\": \"master\",\n"
        "  \"name\": \"Release version 3.4.0\",\n"
        "  \"draft\": false,\n"
        "  \"author\": {\n"
        "    \"login\": \"peterfpeterson\",\n"
        "    \"id\": 404003,\n"
        "    \"avatar_url\": "
        "\"https://avatars.githubusercontent.com/u/404003?v=3\",\n"
        "    \"gravatar_id\": \"\",\n"
        "    \"url\": \"https://api.github.com/users/peterfpeterson\",\n"
        "    \"html_url\": \"https://github.com/peterfpeterson\",\n"
        "    \"followers_url\": "
        "\"https://api.github.com/users/peterfpeterson/followers\",\n"
        "    \"following_url\": "
        "\"https://api.github.com/users/peterfpeterson/following{/"
        "other_user}\",\n"
        "    \"gists_url\": "
        "\"https://api.github.com/users/peterfpeterson/gists{/gist_id}\",\n"
        "    \"starred_url\": "
        "\"https://api.github.com/users/peterfpeterson/starred{/owner}{/"
        "repo}\",\n"
        "    \"subscriptions_url\": "
        "\"https://api.github.com/users/peterfpeterson/subscriptions\",\n"
        "    \"organizations_url\": "
        "\"https://api.github.com/users/peterfpeterson/orgs\",\n"
        "    \"repos_url\": "
        "\"https://api.github.com/users/peterfpeterson/repos\",\n"
        "    \"events_url\": "
        "\"https://api.github.com/users/peterfpeterson/events{/privacy}\",\n"
        "    \"received_events_url\": "
        "\"https://api.github.com/users/peterfpeterson/received_events\",\n"
        "    \"type\": \"User\",\n"
        "    \"site_admin\": false\n"
        "  }\n"
        "}";

    return outputString;
  }
  std::string getCurrentVersion() const override { return CurrentVersion; }
};
} // namespace

class CheckMantidVersionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CheckMantidVersionTest *createSuite() {
    return new CheckMantidVersionTest();
  }
  static void destroySuite(CheckMantidVersionTest *suite) { delete suite; }

  void test_Init() {
    CheckMantidVersion alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void runTest(const std::string &localVersion,
               const std::string &gitHubVersion, bool expectedResult) {
    MockedCheckMantidVersion alg(localVersion, gitHubVersion);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())

    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    std::string currentVersion =
        alg.PropertyManagerOwner::getProperty("CurrentVersion");
    std::string mostRecentVersion =
        alg.PropertyManagerOwner::getProperty("MostRecentVersion");
    bool isNewVersionAvailable =
        alg.PropertyManagerOwner::getProperty("IsNewVersionAvailable");

    // Check the results
    TS_ASSERT_EQUALS(alg.CurrentVersion, currentVersion);
    TS_ASSERT_EQUALS(expectedResult, isNewVersionAvailable);
  }

  void test_execLocalNewerRevision() { runTest("3.4.2", "v3.4.0", false); }
  void test_execRemoteNewerRevision() { runTest("3.4.0", "v3.4.1", true); }
  void test_execLocaldevelopRevision() {
    runTest("3.4.20150703.1043", "v3.4.0", false);
  }
  void test_execLocaldevelopNewerRevision() {
    runTest("3.4.20150703.1043", "v3.4.1", false);
  }

  void test_execLocalNewerMinor() { runTest("3.5.2", "v3.4.7", false); }
  void test_execRemoteNewerMinor() { runTest("3.3.7", "v3.4.1", true); }
  void test_execLocalNewerMajor() { runTest("2.0.2", "v1.11.7", false); }
  void test_execRemoteNewerMajor() { runTest("2.3.7", "v3.0.0", true); }
};

#endif /* MANTID_DATAHANDLING_CHECKMANTIDVERSIONTEST_H_ */
