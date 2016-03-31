#ifndef MANTID_CUSTOMINTERFACES_ALCLATESTFILEFINDERTEST_H_
#define MANTID_CUSTOMINTERFACES_ALCLATESTFILEFINDERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/make_unique.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidQtCustomInterfaces/Muon/ALCLatestFileFinder.h"
#include "MantidTestHelpers/ScopedFileHelper.h"

#include <Poco/File.h>
#include <Poco/DateTime.h>

using MantidQt::CustomInterfaces::ALCLatestFileFinder;
using ScopedFileHelper::ScopedFile;
using Mantid::Kernel::DateAndTime;

/**
 * Extension of ScopedFile used for testing purposes
 */
class TestFile {
public:
  /// Constructor which creates valid filename
  TestFile(const std::string &time, const std::string &instrument,
           const std::string &run, const std::string &extension = "nxs")
      : m_file("", createFileName(instrument, run, extension)) {
    adjustFileTime(m_file.getFileName(), time);
  }
  /// Constructor taking any filename
  TestFile(const std::string &time, const std::string &name)
      : m_file("", name) {
    adjustFileTime(m_file.getFileName(), time);
  }
  std::string getFileName() { return m_file.getFileName(); };

private:
  /**
 * Generate a filename from supplied instrument, run number
 * @param instrument [input] :: instrument name
 * @param run [input] :: run number
 * @param extension [input] :: extension
 * @returns :: filename
 */
  std::string createFileName(const std::string &instrument,
                             const std::string &run,
                             const std::string &extension) {
    static const size_t numberLength = 8;
    std::ostringstream stream;
    stream << instrument;
    const size_t numZeros = numberLength - run.size();
    for (size_t i = 0; i < numZeros; i++) {
      stream << "0";
    }
    stream << run;
    stream << "." << extension;
    return stream.str();
  }
  /**
 * Set file's last modified time (resolution: nearest second)
 * @param path :: [input] Path to file
 * @param time :: [input] ISO8601 formatted time string
 */
  void adjustFileTime(const std::string &path,
                      const std::string &modifiedTime) {
    // Make sure the file exists
    Poco::File file(path);
    TS_ASSERT(file.exists() && file.canWrite() && file.isFile());

    // Parse the time string and convert to Poco's format
    // Ignore sub-second intervals
    DateAndTime time(modifiedTime);
    Poco::DateTime pocoTime(time.year(), time.month(), time.day(), time.hour(),
                            time.minute(), time.second());

    // Set the file's last modified time
    file.setLastModified(pocoTime.timestamp());
  }
  ScopedFile m_file;
};

class ALCLatestFileFinderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ALCLatestFileFinderTest *createSuite() {
    return new ALCLatestFileFinderTest();
  }
  static void destroySuite(ALCLatestFileFinderTest *suite) { delete suite; }

  /**
   * Test finding the most recent file in the directory
   * Should deal with adding and removing files
   */
  void test_getMostRecentFile() {
    auto files = generateTestFiles();
    ALCLatestFileFinder finder(files[0].getFileName());
    TS_ASSERT_EQUALS(finder.getMostRecentFile(), files[1].getFileName());
    { // file added
      auto newFile = TestFile("2116-03-15T15:00:00", "MUSR", "90003");
      TS_ASSERT_EQUALS(finder.getMostRecentFile(), newFile.getFileName());
    }
    // file removed (newFile went out of scope)
    TS_ASSERT_EQUALS(finder.getMostRecentFile(), files[1].getFileName());
  }

  /**
   * Test that the finder ignores non-NeXus files
   */
  void test_ignoreNonNeXus() {
    auto files = generateTestFiles();
    auto nonNexus = TestFile("2116-03-15T16:00:00", "MUSR", "90004", "run");
    ALCLatestFileFinder finder(files[0].getFileName());
    TS_ASSERT_EQUALS(finder.getMostRecentFile(), files[1].getFileName());
  }

  /**
   * Test that the finder ignores NeXus files from the wrong instrument
   */
  void test_ignoreWrongInstrument() {
    auto files = generateTestFiles();
    auto wrongInstrument = TestFile("2116-03-15T16:00:00", "EMU", "80000");
    ALCLatestFileFinder finder(files[0].getFileName());
    std::string foundFile;
    TS_ASSERT_THROWS_NOTHING(foundFile = finder.getMostRecentFile());
    TS_ASSERT_EQUALS(foundFile, files[1].getFileName());
  }

  /**
   * Test that the finder ignores "invalid" NeXus files
   */
  void test_ignoreInvalidNeXus() {
    auto files = generateTestFiles();
    auto badNexus = TestFile("2116-03-15T16:00:00", "ALCResults.nxs");
    ALCLatestFileFinder finder(files[0].getFileName());
    std::string foundFile;
    TS_ASSERT_THROWS_NOTHING(foundFile = finder.getMostRecentFile());
    TS_ASSERT_EQUALS(foundFile, files[1].getFileName());
  }

private:
  /**
   * Generate three scoped test files
   * The second file is the most recent
   * @returns :: vector containing three files
   */
  std::vector<TestFile> generateTestFiles() {
    std::vector<TestFile> files;
    // 100 years so it won't clash with other files in temp directory
    files.emplace_back("2116-03-15T12:00:00", "MUSR", "90000");
    files.emplace_back("2116-03-15T14:00:00", "MUSR", "90001");
    files.emplace_back("2116-03-15T13:00:00", "MUSR", "90002");
    return files;
  }
};

#endif /* MANTID_CUSTOMINTERFACES_ALCLATESTFILEFINDERTEST_H_ */