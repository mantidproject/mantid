// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ISISDATAARCHIVETEST_H_
#define ISISDATAARCHIVETEST_H_

#include <cxxtest/TestSuite.h>
#include <fstream>
#include <sstream>

#include "MantidAPI/ArchiveSearchFactory.h"
#include "MantidDataHandling/ISISDataArchive.h"

using namespace Mantid::DataHandling;
using namespace Mantid::API;

namespace {
#ifdef _WIN32
const char *URL_PREFIX = "http://data.isis.rl.ac.uk/where.py/windir?name=";
#else
const char *URL_PREFIX = "http://data.isis.rl.ac.uk/where.py/unixdir?name=";
#endif
}

class MockOutRequests : public ISISDataArchive {
public:
  MockOutRequests()
      : m_sendRequestReturnVal("/archive/default/path"),
        m_mockFileExists(true) {}

  void setSendRequestReturnVal(std::string &return_val) {
    m_sendRequestReturnVal = return_val;
  }

  void setFileExists(const bool doesFileExist) {
    m_mockFileExists = doesFileExist;
  }

protected:
  /**
   * Mocked out sendRequest.
   */
  std::ostringstream sendRequest(const std::string &fName) const override {
    (void)fName;
    std::ostringstream os;
    os << m_sendRequestReturnVal;
    return os;
  }

  /**
   * Mocked out fileExists.
   * Mocks search for files with extensions.
   * This is a simplistic version. Will return m_mockFileExists
   * for any `path`.
   * Except for hard-coded case of path ending in .txt
   * This exception is to test that
   * `getCorrectExtension` will loop until it finds
   * the first acceptable extension.
   * @param :: path to file, including extension
   */
  bool fileExists(const std::string &path) const override {
    if (path.substr(path.size() - 4, 4) == ".txt") {
      return false;
    }
    return m_mockFileExists;
  }

private:
  // Mimics the directory tree returned by sendRequest
  // e.g. /archive/ndxloq/Instrument/data/cycle_98_0
  std::string m_sendRequestReturnVal;

  // Used in mocking fileExists.
  bool m_mockFileExists;
};

class ISISDataArchiveTest : public CxxTest::TestSuite {
public:
  void testGetCorrectExtensionsWithCorrectExtensions() {
    std::vector<std::string> exts = {".RAW"};
    std::string filename = "/archive/default/path/hrpd273";

    MockOutRequests arch;
    const std::string actualPath = arch.getCorrectExtension(filename, exts);
    TS_ASSERT_EQUALS(actualPath, "/archive/default/path/hrpd273.RAW");
  }

  void testGetCorrectExtensionsWithInCorrectExtensions() {
    std::vector<std::string> exts = {".RAW"};
    std::string filename = "hrpd273";

    MockOutRequests arch;
    arch.setFileExists(false);

    const std::string actualPath = arch.getCorrectExtension(filename, exts);
    TS_ASSERT_EQUALS(actualPath, "");
  }

  void testGetCorrectExtensionsLoopsUntilFindsFirstCorrectExtension() {
    std::vector<std::string> exts = {".txt", ".RAW"};
    std::string filename = "/archive/default/path/hrpd273";

    MockOutRequests arch;
    const std::string actualPath = arch.getCorrectExtension(filename, exts);
    TS_ASSERT_EQUALS(actualPath, "/archive/default/path/hrpd273.RAW");
  }

  void testFilenameLoopIgnoresEmptyFilenames() {
    std::vector<std::string> exts = {".RAW"};
    std::set<std::string> filenames = {"", "", "", "hrpd273"};

    MockOutRequests arch;
    const std::string actualPath = arch.getArchivePath(filenames, exts);

    std::string expectedPath = "/archive/default/path";
    if (strcmp(URL_PREFIX, "http://data.isis.rl.ac.uk/where.py/windir?name=") ==
        0) {
      expectedPath += "\\";
    } else {
      expectedPath += "/";
    }
    TS_ASSERT_EQUALS(actualPath, expectedPath + "hrpd273.RAW");
  }

  void testGetArchivePathReturnsEmptyStringIfNoFileFound() {
    std::vector<std::string> exts = {".RAW", ".log"};
    std::set<std::string> filenames = {"hrpd280", "hrpd273"};

    MockOutRequests arch;
    arch.setFileExists(false);
    const std::string actualPath = arch.getArchivePath(filenames, exts);
    TS_ASSERT_EQUALS(actualPath, "");
  }

  void testFactory() {
    boost::shared_ptr<IArchiveSearch> arch =
        ArchiveSearchFactory::Instance().create("ISISDataSearch");
    TS_ASSERT(arch);
  }

  /*****UN 'x' THE FOLLOWING TESTS WHEN TESTING LOCALLY*****/
  /**
   * Un-'x' this test name to test locally.
   * This tests the file extensions loop.
   * To run, this tests requires that the ISIS archive is on your local machine.
   */
  void xtestgetCorrectExtensionWithCorrectExtensionWithWebCall() {
    std::string path;
    if (strcmp(URL_PREFIX, "http://data.isis.rl.ac.uk/where.py/windir?name=") ==
        0) {
      path = "\\isis.cclrc.ac.uk\\inst$\\ndxhrpd\\instrument\\data\\cycle_98_"
             "0\\HRP00273";
    } else {
      path = "/archive/ndxhrpd/Instrument/data/cycle_98_0/HRP00273";
    }
    ISISDataArchive arch;

    const std::vector<std::string> correct_exts = {".RAW"};
    const std::string actualResult =
        arch.getCorrectExtension(path, correct_exts);
    TS_ASSERT_EQUALS(actualResult, path + ".RAW");
  }

  void xtestgetCorrectExtensionWithInCorrectExtensionsWithWebCall() {
    std::string path;
    if (strcmp(URL_PREFIX, "http://data.isis.rl.ac.uk/where.py/windir?name=") ==
        0) {
      path = "\\isis.cclrc.ac.uk\\inst$\\ndxhrpd\\instrument\\data\\cycle_98_"
             "0\\HRP00273";
    } else {
      path = "/archive/ndxhrpd/Instrument/data/cycle_98_0/HRP00273";
    }
    ISISDataArchive arch;

    const std::vector<std::string> incorrect_exts = {".so", ".txt"};
    const std::string actualResult =
        arch.getCorrectExtension(path, incorrect_exts);
    TS_ASSERT_EQUALS(actualResult, "");
  }
};

#endif /*ISISDATAARCHIVETEST_H_*/
