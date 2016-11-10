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
  TestFile(const std::string &time, const std::string &directory,
           const std::string &instrument, const std::string &run,
           const std::string &extension = "nxs")
      : m_file("", createFileName(directory, instrument, run, extension)) {
    adjustFileTime(m_file.getFileName(), time);
  }
  /// Constructor taking any filename
  TestFile(const std::string &time, const std::string &directory,
           const std::string &name)
      : m_file("", directory + Poco::Path::separator() + name) {
    adjustFileTime(m_file.getFileName(), time);
  }
  std::string getFileName() { return m_file.getFileName(); };

private:
  /**
 * Generate a filename from supplied instrument, run number
 * @param directory :: [input] Name of directory to create files in (must
 * already exist)
 * @param instrument [input] :: instrument name
 * @param run [input] :: run number
 * @param extension [input] :: extension
 * @returns :: filename
 */
  std::string createFileName(const std::string &directory,
                             const std::string &instrument,
                             const std::string &run,
                             const std::string &extension) {
    static const size_t numberLength = 8;
    std::ostringstream stream;
    stream << directory << Poco::Path::separator();
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
    const std::string dirName("test_getMostRecentFile");
    auto files = generateTestFiles(dirName);
    ALCLatestFileFinder finder(files[0].getFileName());
    TS_ASSERT_EQUALS(finder.getMostRecentFile(), files[2].getFileName());
    { // file added
      auto newFile = TestFile("2116-03-15T15:00:00", dirName, "MUSR", "90003");
      TS_ASSERT_EQUALS(finder.getMostRecentFile(), newFile.getFileName());
    }
    // file removed (newFile went out of scope)
    TS_ASSERT_EQUALS(finder.getMostRecentFile(), files[2].getFileName());
  }

  /**
   * Test that the finder ignores non-NeXus files
   */
  void test_ignoreNonNeXus() {
    const std::string dirName("test_ignoreNonNeXus");
    auto files = generateTestFiles(dirName);
    auto nonNexus =
        TestFile("2116-03-15T16:00:00", dirName, "MUSR", "90004", "run");
    ALCLatestFileFinder finder(files[0].getFileName());
    TS_ASSERT_EQUALS(finder.getMostRecentFile(), files[2].getFileName());
  }

  /**
   * Test that the finder ignores NeXus files from the wrong instrument
   */
  void test_ignoreWrongInstrument() {
    const std::string &dirName("test_ignoreWrongInstrument");
    auto files = generateTestFiles(dirName);
    auto wrongInstrument =
        TestFile("2116-03-15T16:00:00", dirName, "EMU", "80000");
    ALCLatestFileFinder finder(files[0].getFileName());
    std::string foundFile;
    TS_ASSERT_THROWS_NOTHING(foundFile = finder.getMostRecentFile());
    TS_ASSERT_EQUALS(foundFile, files[2].getFileName());
  }

  /**
   * Test that the finder ignores "invalid" NeXus files
   */
  void test_ignoreInvalidNeXus() {
    const std::string &dirName("test_ignoreInvalidNeXus");
    auto files = generateTestFiles(dirName);
    auto badNexus = TestFile("2116-03-15T16:00:00", dirName, "ALCResults.nxs");
    ALCLatestFileFinder finder(files[0].getFileName());
    std::string foundFile;
    TS_ASSERT_THROWS_NOTHING(foundFile = finder.getMostRecentFile());
    TS_ASSERT_EQUALS(foundFile, files[2].getFileName());
  }

private:
  /**
   * Generate three scoped test files
   * The creation dates go in run number order, as is the case with real files
   * (confirmed with scientists that this is always the case)
   * @param directory :: [input] Name of directory to create files in (will be
   * created if it doesn't exist)
   * @returns :: vector containing three files
   */
  std::vector<TestFile> generateTestFiles(const std::string &directory) {
    // First ensure the directory exists
    Poco::Path tmpPath(Mantid::Kernel::ConfigService::Instance().getTempDir());
    tmpPath.pushDirectory(directory);
    Poco::File tmpDir(tmpPath);
    tmpDir.createDirectories();
    // Now create the files
    std::vector<TestFile> files;
    // 100 years so it won't clash with other files in temp directory
    files.emplace_back("2116-03-15T12:00:00", directory, "MUSR", "90000");
    files.emplace_back("2116-03-15T13:00:00", directory, "MUSR", "90001");
    files.emplace_back("2116-03-15T14:00:00", directory, "MUSR", "90002");
    return files;
  }
};

/// Performance tests
class ALCLatestFileFinderTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ALCLatestFileFinderTestPerformance *createSuite() {
    return new ALCLatestFileFinderTestPerformance();
  }
  static void destroySuite(ALCLatestFileFinderTestPerformance *suite) {
    delete suite;
  }

  ALCLatestFileFinderTestPerformance() {
    for (size_t i = 10; i < 59; i++) {
      std::ostringstream time, run;
      time << "2116-03-16T18:00:" << i;
      run << "900" << i;
      m_files.emplace_back(time.str(), "MUSR", run.str());
    }
  }

  // Set up files here - will be deleted when the class is destroyed
  void setUp() override {}

  void tearDown() override {
    TS_ASSERT_EQUALS(m_mostRecent, m_files.back().getFileName());
  }

  void test_latestFileFinder_performance() {
    ALCLatestFileFinder finder(m_files[0].getFileName());
    m_mostRecent = finder.getMostRecentFile();
  }

private:
  std::vector<TestFile> m_files;
  std::string m_mostRecent;
};

#endif /* MANTID_CUSTOMINTERFACES_ALCLATESTFILEFINDERTEST_H_ */