// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include <utility>

#include "MantidDataHandling/CheckMantidVersion.h"

using Mantid::DataHandling::CheckMantidVersion;
using namespace Mantid::API;

namespace {
/**
 * Mock out the version calls of this algorithm
 */
class MockedCheckMantidVersion : public CheckMantidVersion {
public:
  MockedCheckMantidVersion(std::string currentVersion, std::string gitHubVersion)
      : CheckMantidVersion(), CurrentVersion(std::move(currentVersion)), GitHubVersion(std::move(gitHubVersion)) {}

  std::string CurrentVersion;
  std::string GitHubVersion;

  // It's useful to be able to test the output of this helper function, so make it public in our mocked class.
  using CheckMantidVersion::splitVersionString;

private:
  std::string getVersionsFromGitHub(const std::string &url) override {
    // The initial assignment of the value to url is just to suppress a compiler
    // warning.
    std::string outputString(url);
    outputString = "{\n"
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
  static CheckMantidVersionTest *createSuite() { return new CheckMantidVersionTest(); }
  static void destroySuite(CheckMantidVersionTest *suite) { delete suite; }

  void test_Init() {
    CheckMantidVersion alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void runTest(const std::string &localVersion, const std::string &gitHubVersion, bool expectedResult) {
    MockedCheckMantidVersion alg(localVersion, gitHubVersion);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())

    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    std::string currentVersion = alg.getProperty("CurrentVersion");
    alg.getProperty("MostRecentVersion");
    bool isNewVersionAvailable = alg.getProperty("IsNewVersionAvailable");

    // Check the results
    TS_ASSERT_EQUALS(alg.CurrentVersion, currentVersion);
    TS_ASSERT_EQUALS(expectedResult, isNewVersionAvailable);
  }

  void runSplitVersionString(const std::string &version, const std::vector<int> &expectedResult) {
    MockedCheckMantidVersion alg("dummy", "dummy");
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_EQUALS(alg.splitVersionString(version), expectedResult);
  }

  void test_execLocalNewerRevision() { runTest("3.4.2", "v3.4.0", false); }
  void test_execRemoteNewerRevision() { runTest("3.4.0", "v3.4.1", true); }
  void test_execLocalNightlyRevision() { runTest("3.4.20150703.1043", "v3.4.0", false); }
  void test_execLocalNightlyNewerRevision() { runTest("3.4.20150703.1043", "v3.4.1", false); }

  void test_execLocalNewerMinor() { runTest("3.5.2", "v3.4.7", false); }
  void test_execRemoteNewerMinor() { runTest("3.3.7", "v3.4.1", true); }
  void test_execLocalNewerMajor() { runTest("2.0.2", "v1.11.7", false); }
  void test_execRemoteNewerMajor() { runTest("2.3.7", "v3.0.0", true); }

  // PEP 440 version scheme for "local" versions with arbitrary alphanumeric characters after a "+"
  // should work as if they don't have the + and anything afterwards.
  void test_tweakLocalNewer() { runTest("1.2.4+tweak.morestuff", "v1.2.3", false); }
  void test_tweakRemoteNewer() { runTest("1.2.3+tweak.morestuff", "v3.0.0", true); }

  void test_devLocalNewer() { runTest("1.2.3.dev55", "v1.2.1", false); }
  void test_devRemoteNewer() { runTest("1.2.3.dev55", "v1.4.7", true); }

  void test_devUncommittedLocalNewer() { runTest("1.2.3.dev55+uncommitted", "v1.2.1", false); }
  void test_devUncommittedRemoteNewer() { runTest("1.2.3.dev55+uncommitted", "v1.4.7", true); }

  // Want anyhting other than numbers to be ignored when splitting the version string into numbers.
  void test_tweakVersionNumber() { runSplitVersionString("1.2.3+tweak.morestuff", {1, 2, 3}); }
  void test_devVersionNumber() { runSplitVersionString("1.2.3.dev55", {1, 2, 3}); }
  void test_devUncommittedVersionNumber() { runSplitVersionString("1.2.3.dev55+uncommitted", {1, 2, 3}); }
  void test_releaseCandidateVersionNumber() { runSplitVersionString("1.2.3rc1", {1, 2, 3}); }
};
