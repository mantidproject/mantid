#ifndef MANTID_DATAHANDLING_STARTANDENDTIMEFROMNEXUSFILEEXTRACTORTEST_H_
#define MANTID_DATAHANDLING_STARTANDENDTIMEFROMNEXUSFILEEXTRACTORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FileFinder.h"
#include "MantidDataHandling/StartAndEndTimeFromNexusFileExtractor.h"
#include <string>

using Mantid::DataHandling::StartAndEndTimeFromNexusFileExtractor;

class StartAndEndTimeFromNexusFileExtractorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StartAndEndTimeFromNexusFileExtractorTest *createSuite() {
    return new StartAndEndTimeFromNexusFileExtractorTest();
  }
  static void destroySuite(StartAndEndTimeFromNexusFileExtractorTest *suite) {
    delete suite;
  }

  void test_that_throws_for_non_sense_file() {
    // Arrrange
    StartAndEndTimeFromNexusFileExtractor extractor;
    std::string filename = "file_doesnt_exist.nxs";

    // Act + Assert
    TSM_ASSERT_THROWS_ANYTHING(
        "Should throw something when opening file which does not exist",
        extractor.extractEndTime(filename));
  }

  void test_that_times_can_be_extracted_from_muon_file() {
    // Arrange
    StartAndEndTimeFromNexusFileExtractor extractor;
    std::string filename = "emu00006473.nxs";
    auto fullFilePath =
        Mantid::API::FileFinder::Instance().getFullPath(filename);

    // Act
    auto startTime = extractor.extractStartTime(fullFilePath);
    auto endTime = extractor.extractEndTime(fullFilePath);

    // Assert
    Mantid::Kernel::DateAndTime expectedStartTimeString("2006-11-21T07:04:30");
    Mantid::Kernel::DateAndTime expectedEndTimeString("2006-11-21T09:29:28");
    TSM_ASSERT("Should have the same start time",
               startTime == expectedStartTimeString);
    TSM_ASSERT("Should have the same end time",
               endTime == expectedEndTimeString);
  }

  void test_that_times_can_be_extracted_from_isis_file() {
    // Arrange
    StartAndEndTimeFromNexusFileExtractor extractor;
    std::string filename = "POLREF00014966.nxs";
    auto fullFilePath =
        Mantid::API::FileFinder::Instance().getFullPath(filename);

    // Act
    auto startTime = extractor.extractStartTime(fullFilePath);
    auto endTime = extractor.extractEndTime(fullFilePath);

    // Assert
    Mantid::Kernel::DateAndTime expectedStartTimeString("2015-10-13T05:34:32");
    Mantid::Kernel::DateAndTime expectedEndTimeString("2015-10-13T11:30:28");
    TSM_ASSERT("Should have the same start time",
               startTime == expectedStartTimeString);
    TSM_ASSERT("Should have the same end time",
               endTime == expectedEndTimeString);
  }

  void test_that_times_can_be_extracted_from_processed_file() {
    // Arrange
    StartAndEndTimeFromNexusFileExtractor extractor;
    std::string filename = "LOQ48127.nxs";
    auto fullFilePath =
        Mantid::API::FileFinder::Instance().getFullPath(filename);

    // Act
    auto startTime = extractor.extractStartTime(fullFilePath);
    auto endTime = extractor.extractEndTime(fullFilePath);

    // Assert
    Mantid::Kernel::DateAndTime expectedStartTimeString("2008-12-18T17:58:38");
    Mantid::Kernel::DateAndTime expectedEndTimeString("2008-12-18T18:06:20");
    TSM_ASSERT("Should have the same start time",
               startTime == expectedStartTimeString);
    TSM_ASSERT("Should have the same end time",
               endTime == expectedEndTimeString);
  }

  void test_that_times_can_be_extracted_from_tof_raw_file() {
    // Arrange
    StartAndEndTimeFromNexusFileExtractor extractor;
    std::string filename = "REF_L_32035.nxs";
    auto fullFilePath =
        Mantid::API::FileFinder::Instance().getFullPath(filename);

    // Act
    auto startTime = extractor.extractStartTime(fullFilePath);
    auto endTime = extractor.extractEndTime(fullFilePath);

    // Assert
    Mantid::Kernel::DateAndTime expectedStartTimeString(
        "2010-06-09T14:29:31-04:00");
    Mantid::Kernel::DateAndTime expectedEndTimeString(
        "2010-06-09T14:29:07-04:00");

    TSM_ASSERT("Should have the same start time",
               startTime == expectedStartTimeString);
    TSM_ASSERT("Should have the same end time",
               endTime == expectedEndTimeString);
  }
};

#endif /* MANTID_DATAHANDLING_STARTANDENDTIMEFROMNEXUSFILEEXTRACTORTEST_H_ */