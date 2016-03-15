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
   * Should ignore non-NeXus files
   */
  void test_getMostRecentFile() {
    // 100 years so it won't clash with other files in temp directory
    auto file0 = ScopedFile("", "0.nxs");
    adjustFileTime(file0.getFileName(), "2116-03-15T12:00:00");
    auto file1 = ScopedFile("", "1.nxs");
    adjustFileTime(file1.getFileName(), "2116-03-15T14:00:00");
    auto file2 = ScopedFile("", "2.nxs");
    adjustFileTime(file2.getFileName(), "2116-03-15T13:00:00");
    ALCLatestFileFinder finder(file0.getFileName());
    TS_ASSERT_EQUALS(finder.getMostRecentFile(), file1.getFileName());
    { // file added
      auto file3 = ScopedFile("", "3.nxs");
      adjustFileTime(file3.getFileName(), "2116-03-15T15:00:00");
      TS_ASSERT_EQUALS(finder.getMostRecentFile(), file3.getFileName());
    }
    // file removed (file3 went out of scope)
    TS_ASSERT_EQUALS(finder.getMostRecentFile(), file1.getFileName());
    auto nonNexus = ScopedFile("", "4.run");
    adjustFileTime(nonNexus.getFileName(), "2116-03-15T16:00:00");
    TS_ASSERT_EQUALS(finder.getMostRecentFile(), file1.getFileName());
  }

private:
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
};

#endif /* MANTID_CUSTOMINTERFACES_ALCLATESTFILEFINDERTEST_H_ */