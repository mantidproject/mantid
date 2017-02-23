#ifndef MANTID_DATAHANDLING_DOWNLOADINSTRUMENTTEST_H_
#define MANTID_DATAHANDLING_DOWNLOADINSTRUMENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/DownloadInstrument.h"
#include "MantidKernel/ConfigService.h"

#include <Poco/Net/HTTPResponse.h>
#include <Poco/Glob.h>
#include <Poco/File.h>
#include <Poco/Path.h>

#include <fstream>
#include <string>
#include <cstdio>

using Mantid::DataHandling::DownloadInstrument;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

namespace {
/**
 * Mock out the internet calls of this algorithm
 */
class MockedDownloadInstrument : public DownloadInstrument {
public:
  MockedDownloadInstrument() : DownloadInstrument() {}

private:
  int doDownloadFile(
      const std::string &urlFile, const std::string &localFilePath = "",
      const StringToStringMap &headers = StringToStringMap()) override {
    std::string dateTime;
    auto it = headers.find("if-modified-since");
    if (it != headers.end())
      dateTime = it->second;

    std::string outputString;
    if (urlFile.find("api.github.com") != std::string::npos) {
      outputString =
          "[\n"
          "  {\n"
          "    \"name\": \"NewFile.xml\",\n"
          "    \"path\": \"instrument/NewFile.xml\",\n"
          "    \"sha\": \"Xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\",\n"
          "    \"size\": 60,\n"
          "    \"url\": \"https://www.mantidproject.org/invalid\",\n"
          "    \"html_url\": \"https://www.mantidproject.org/NewFile.xml\",\n"
          "    \"git_url\": \"https://www.mantidproject.org/invalid\",\n"
          "    \"type\": \"file\",\n"
          "    \"_links\": {\n"
          "      \"self\": \"https://www.mantidproject.org/invalid\",\n"
          "      \"git\": \"https://www.mantidproject.org/invalid\",\n"
          "      \"html\": \"https://www.mantidproject.org/invalid\"\n"
          "    }\n"
          "  },\n"
          "  {\n"
          "    \"name\": \"UpdatableFile.xml\",\n"
          "    \"path\": \"instrument/UpdatableFile.xml\",\n"
          "    \"sha\": \"d66ba0a04290093d83d41901048068d495d41764\",\n"
          "    \"size\": 106141,\n"
          "    \"url\": \"https://www.mantidproject.org/invalid\",\n"
          "    \"html_url\": "
          "\"https://www.mantidproject.org/UpdatableFile.xml\",\n"
          "    \"git_url\": \"https://www.mantidproject.org/invalid\",\n"
          "    \"type\": \"file\",\n"
          "    \"_links\": {\n"
          "      \"self\": \"https://www.mantidproject.org/invalid\",\n"
          "      \"git\": \"https://www.mantidproject.org/invalid\",\n"
          "      \"html\": \"https://www.mantidproject.org/invalid\"\n"
          "    }\n"
          "  }\n"
          "]";
    } else if (urlFile.find("https://www.mantidproject.org/NewFile.xml") !=
               std::string::npos) {
      outputString = "Here is some sample text for NewFile.xml";
    } else if (urlFile.find(
                   "https://www.mantidproject.org/UpdatableFile.xml") !=
               std::string::npos) {
      outputString = "Here is some sample text for WISH_Definition.xml";
    }

    std::ofstream file;
    file.open(localFilePath.c_str());
    file << outputString;
    file.close();

    return Poco::Net::HTTPResponse::HTTP_FOUND;
  }
};
}

class DownloadInstrumentTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DownloadInstrumentTest *createSuite() {
    return new DownloadInstrumentTest();
  }
  static void destroySuite(DownloadInstrumentTest *suite) { delete suite; }

  void createDirectory(Poco::Path path) {
    Poco::File file(path);
    if (file.createDirectory()) {
      m_directoriesToRemove.push_back(file);
    }
  }

  void removeDirectories() {
    for (auto directory : m_directoriesToRemove) {
      try {
        directory.remove(true);
      } catch (Poco::FileException &fe) {
        std::cout << fe.what() << std::endl;
      }
    }
    m_directoriesToRemove.clear();
  }

  void setUp() override {
    const std::string TEST_SUFFIX = "TEMPORARY_unitTest";
    m_originalInstDir =
        Mantid::Kernel::ConfigService::Instance().getInstrumentDirectories();

    // change the local download directory by adding a unittest subdirectory
    auto testDirectories = m_originalInstDir;
    Poco::Path localDownloadPath(m_originalInstDir[0]);
    localDownloadPath.pushDirectory(TEST_SUFFIX);
    m_localInstDir = localDownloadPath.toString();
    createDirectory(localDownloadPath);
    testDirectories[0] = m_localInstDir;

    // also if you move the instrument directory to one with less files then it
    // will run faster as it does not need to checksum as many files
    try {
      Poco::Path installInstrumentPath(testDirectories.back());
      installInstrumentPath.pushDirectory(TEST_SUFFIX);
      createDirectory(installInstrumentPath);
      testDirectories.back() = installInstrumentPath.toString();
    } catch (Poco::FileException &) {
      std::cout << "Failed to change instrument directory continuing without, "
                   "fine, just slower\n";
    }

    Mantid::Kernel::ConfigService::Instance().setInstrumentDirectories(
        testDirectories);
  }

  void tearDown() override {
    Mantid::Kernel::ConfigService::Instance().setInstrumentDirectories(
        m_originalInstDir);
    removeDirectories();
  }

  void test_Init() {
    MockedDownloadInstrument alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  // These tests create some files, but they entire directories are created and
  // removed in setup and teardown
  void test_exec() {
    TSM_ASSERT_EQUALS("The expected number of files downloaded was wrong.",
                      runDownloadInstrument(), 2);
  }

  void test_execOrphanedFile() {
    // add an orphaned file
    Poco::Path orphanedFilePath(m_localInstDir);
    orphanedFilePath.makeDirectory();
    orphanedFilePath.setFileName("Orphaned_Should_not_be_here.xml");

    std::ofstream file;
    file.open(orphanedFilePath.toString().c_str());
    file.close();

    TSM_ASSERT_EQUALS("The expected number of files downloaded was wrong.",
                      runDownloadInstrument(), 2);

    Poco::File orphanedFile(orphanedFilePath);
    TSM_ASSERT("The orphaned file was not deleted",
               orphanedFile.exists() == false);
  }

  int runDownloadInstrument() {
    // Name of the output workspace.
    std::string outWSName("DownloadInstrumentTest_OutputWS");

    MockedDownloadInstrument alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    return alg.getProperty("FileDownloadCount");
  }

  std::string m_localInstDir;
  std::vector<std::string> m_originalInstDir;
  std::vector<Poco::File> m_directoriesToRemove;
};

#endif /* MANTID_DATAHANDLING_DOWNLOADINSTRUMENTTEST_H_ */
