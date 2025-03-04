// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FileFinder.h"
#include "MantidDataHandling/StartAndEndTimeFromNexusFileExtractor.h"
#include <string>

using namespace Mantid::DataHandling;

class StartAndEndTimeFromNexusFileExtractorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StartAndEndTimeFromNexusFileExtractorTest *createSuite() {
    return new StartAndEndTimeFromNexusFileExtractorTest();
  }
  static void destroySuite(StartAndEndTimeFromNexusFileExtractorTest *suite) { delete suite; }

  void test_that_throws_for_non_sense_file() {
    // Arrrange
    std::string filename = "file_doesnt_exist.nxs";

    // Act + Assert
    TSM_ASSERT_THROWS_ANYTHING("Should throw something when opening file which does not exist",
                               extractEndTime(filename));
  }

  void test_that_times_can_be_extracted_from_isis_file() {
    const std::string filename("POLREF00014966.nxs");
    const std::string startTime = "2015-10-13T05:34:32";
    const std::string endTime = "2015-10-13T11:30:28";

    do_test(filename, startTime, endTime);
  }

  void test_that_times_can_be_extracted_from_processed_file() {
    const std::string filename("LOQ48127.nxs");
    const std::string startTime = "2008-12-18T17:58:38";
    const std::string endTime = "2008-12-18T18:06:20";

    do_test(filename, startTime, endTime);
  }

  void test_that_times_can_be_extracted_from_tof_raw_file() {
    const std::string filename("REF_L_32035.nxs");
    const std::string startTime = "2010-06-09T14:29:31-04:00";
    const std::string endTime = "2010-06-09T14:29:07-04:00";

    do_test(filename, startTime, endTime);
  }

private:
  void do_test(const std::string &filename, const std::string &startTime, const std::string &endTime) {
    // Arrange
    auto fullFilePath = Mantid::API::FileFinder::Instance().getFullPath(filename);

    // Act
    auto startTimeExtracted = extractStartTime(fullFilePath);
    auto endTimeExtracted = extractEndTime(fullFilePath);

    // Assert
    Mantid::Types::Core::DateAndTime expectedStartTimeString(startTime.c_str());
    Mantid::Types::Core::DateAndTime expectedEndTimeString(endTime.c_str());

    TSM_ASSERT("Should have the same start time", startTimeExtracted == expectedStartTimeString);
    TSM_ASSERT("Should have the same end time", endTimeExtracted == expectedEndTimeString);
  }
};
