#ifndef MANTID_DATAHANDLING_DOWNLOADINSTRUMENTTEST_H_
#define MANTID_DATAHANDLING_DOWNLOADINSTRUMENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/DownloadInstrument.h"

#include <Poco/Net/HTTPResponse.h>
#include <Poco/Glob.h>
#include <Poco/File.h>
#include <Poco/Path.h>

#include <iostream>
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
  virtual int
  doDownloadFile(const std::string &urlFile,
                 const std::string &localFilePath = "",
                 const StringToStringMap &headers = StringToStringMap()) {
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
          "    \"path\": \"Code/Mantid/instrument/NewFile.xml\",\n"
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
          "    \"path\": \"Code/Mantid/instrument/UpdatableFile.xml\",\n"
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

  void test_Init() {
    MockedDownloadInstrument alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    std::string localInstDir =
        Mantid::Kernel::ConfigService::Instance().getInstrumentDirectories()[0];
    cleanupDiretory(localInstDir);

    TSM_ASSERT_EQUALS("The expected number of files downloaded was wrong.",
                      runDownloadInstrument(), 2);

    cleanupDiretory(localInstDir);
  }

  void test_execTwoTimesInARow() {
    std::string localInstDir =
        Mantid::Kernel::ConfigService::Instance().getInstrumentDirectories()[0];
    cleanupDiretory(localInstDir);

    cleanupDiretory(localInstDir);
  }

  void test_execOrphanedFile() {
    std::string localInstDir =
        Mantid::Kernel::ConfigService::Instance().getInstrumentDirectories()[0];
    cleanupDiretory(localInstDir);

    // add an orphaned file
    Poco::Path orphanedFilePath(localInstDir);
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

    deleteFile(orphanedFilePath.toString());
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

  void cleanupDiretory(std::string dir) {
    Poco::Path path(dir);
    path.makeDirectory();
    deleteFile(path.setFileName("github.json").toString());
    deleteFile(path.setFileName("NewFile.xml").toString());
    deleteFile(path.setFileName("UpdatableFile.xml").toString());
  }

  bool deleteFile(std::string filePath) {
    Poco::File file(filePath);
    if (file.exists()) {
      file.remove();
      return true;
    } else {
      return false;
    }
  }
};

#endif /* MANTID_DATAHANDLING_DOWNLOADINSTRUMENTTEST_H_ */
