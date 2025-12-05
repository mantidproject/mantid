// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/InstrumentFileFinder.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <cxxtest/TestSuite.h>
#include <filesystem>

#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Types::Core::DateAndTime;

class InstrumentFileFinderTest : public CxxTest::TestSuite {
public:
  struct fromToEntry {
    std::string path;
    DateAndTime from;
    DateAndTime to;
  };

  // Test that all the IDFs contain valid-to and valid-from dates and that
  // for a single instrument none of the valid-from dates are equal
  void testAllDatesInIDFs() {

    // Collect all IDF filenames and put them in a multimap where the instrument
    // identifier is the key
    std::unordered_multimap<std::string, fromToEntry> idfFiles;
    std::unordered_set<std::string> idfIdentifiers;

    boost::regex regex(".*_Definition.*\\.xml", boost::regex_constants::icase);
    for (const auto &dir_entry :
         std::filesystem::directory_iterator(ConfigService::Instance().getString("instrumentDefinition.directory"))) {
      const auto &entry_path = dir_entry.path();
      if (!std::filesystem::is_regular_file(entry_path))
        continue;

      std::string l_filenamePart = entry_path.filename().string();

      if (boost::regex_match(l_filenamePart, regex)) {
        std::string validFrom, validTo;
        InstrumentFileFinder::getValidFromTo(entry_path.string(), validFrom, validTo);

        size_t found;
        found = l_filenamePart.find("_Definition");
        fromToEntry ft;
        ft.path = entry_path.string();
        ft.from.setFromISO8601(validFrom);
        // Valid TO is optional
        if (validTo.length() > 0)
          ft.to.setFromISO8601(validTo);
        else
          ft.to.setFromISO8601("2100-01-01T00:00:00");

        idfFiles.emplace(l_filenamePart.substr(0, found), ft);
        idfIdentifiers.insert(l_filenamePart.substr(0, found));
      }
    }

    // iterator to browse through the multimap: paramInfoFromIDF
    std::unordered_multimap<std::string, fromToEntry>::const_iterator it1, it2;
    std::pair<std::unordered_multimap<std::string, fromToEntry>::iterator,
              std::unordered_multimap<std::string, fromToEntry>::iterator>
        ret;

    for (const auto &idfIdentifier : idfIdentifiers) {
      ret = idfFiles.equal_range(idfIdentifier);
      for (it1 = ret.first; it1 != ret.second; ++it1) {
        for (it2 = ret.first; it2 != ret.second; ++it2) {
          if (it1 != it2) {
            // some more intelligent stuff here later
            std::stringstream messageBuffer;
            messageBuffer << "Two IDFs for one instrument have equal valid-from dates"
                          << "IDFs are: " << it1->first << " and " << it2->first
                          << " Date One: " << it1->second.from.toFormattedString()
                          << " Date Two: " << it2->second.from.toFormattedString();
            TSM_ASSERT_DIFFERS(messageBuffer.str(), it2->second.from, it1->second.from);
          }
        }
      }
    }
  }

  void testFindIPF() {
    // Check that instrument dirs are searched correctly
    const std::string expectedFileName = "GEM_parameters.xml";

    const auto result = InstrumentFileFinder::getParameterPath("GEM");
    TS_ASSERT(boost::icontains(result, expectedFileName));

    // Should be case insensitive
    const auto mixedResult = InstrumentFileFinder::getParameterPath("GEM_defINITION.xml");
    TS_ASSERT_EQUALS(result, mixedResult);
  }

  void testFindIPFWithDate() {
    const std::string input = "D2B_Definition_2018-03-01.xml";

    const auto result = InstrumentFileFinder::getParameterPath(input);
    const std::string expected = "D2B_Parameters_2018-03-01.xml";
    TS_ASSERT(boost::icontains(result, expected));
  }

  void testFindIPFNonExistant() {
    const auto result = InstrumentFileFinder::getParameterPath("NotThere");
    TS_ASSERT_EQUALS("", result);
  }

  void testFindIPFWithHint() {
    const auto tmpDir = std::filesystem::temp_directory_path();
    const std::string filename = "test_Parameters.xml";
    const auto expectedPath = tmpDir / filename;

    // Create the file - RAII will close it automatically
    std::ofstream(expectedPath);

    const auto result = InstrumentFileFinder::getParameterPath("test", tmpDir.string());
    // Ensure file was found and it's in the tmp dir
    TS_ASSERT(result.find(filename) != std::string::npos);
    TS_ASSERT(result.find(tmpDir.string()) != std::string::npos);

    std::filesystem::remove(expectedPath);
  }

  void testNonExistantIPFWithHint() {
    const auto tmpDir = std::filesystem::temp_directory_path();
    const auto result = InstrumentFileFinder::getParameterPath("notThere", tmpDir.string());
    TS_ASSERT(result.empty());
  }

  //
  void testHelperFunctions() {
    ConfigService::Instance().updateFacilities();
    InstrumentFileFinder helper;
    std::string boevs = helper.getInstrumentFilename("BIOSANS", "2100-01-31 22:59:59");
    TS_ASSERT(!boevs.empty());
  }

  //
  void testHelper_TOPAZ_No_To_Date() {
    InstrumentFileFinder helper;
    std::string boevs = helper.getInstrumentFilename("TOPAZ", "2011-01-31 22:59:59");
    TS_ASSERT(!boevs.empty());
  }

  void testHelper_ValidDateOverlap() {
    const std::string instDir = ConfigService::Instance().getInstrumentDirectory();
    const std::string testDir = instDir + "unit_testing";
    ConfigService::Instance().setString("instrumentDefinition.directory", testDir);
    InstrumentFileFinder helper;
    std::string boevs = helper.getInstrumentFilename("ARGUS", "1909-01-31 22:59:59");
    TS_ASSERT_DIFFERS(boevs.find("TEST1_ValidDateOverlap"), std::string::npos);
    boevs = helper.getInstrumentFilename("ARGUS", "1909-03-31 22:59:59");
    TS_ASSERT_DIFFERS(boevs.find("TEST2_ValidDateOverlap"), std::string::npos);
    boevs = helper.getInstrumentFilename("ARGUS", "1909-05-31 22:59:59");
    TS_ASSERT_DIFFERS(boevs.find("TEST1_ValidDateOverlap"), std::string::npos);
    ConfigService::Instance().setString("instrumentDefinition.directory", instDir);

    std::vector<std::string> formats = {"xml"};
    std::vector<std::string> dirs;
    dirs.emplace_back(testDir);
    std::vector<std::string> fnames = helper.getResourceFilenames("ARGUS", formats, dirs, "1909-01-31 22:59:59");
    TS_ASSERT_DIFFERS(fnames[0].find("TEST1_ValidDateOverlap"), std::string::npos);
    TS_ASSERT_EQUALS(fnames.size(), 1);
    fnames = helper.getResourceFilenames("ARGUS", formats, dirs, "1909-03-31 22:59:59");
    TS_ASSERT_DIFFERS(fnames[0].find("TEST2_ValidDateOverlap"), std::string::npos);
    TS_ASSERT_DIFFERS(fnames[1].find("TEST1_ValidDateOverlap"), std::string::npos);
    fnames = helper.getResourceFilenames("ARGUS", formats, dirs, "1909-05-31 22:59:59");
    TS_ASSERT_DIFFERS(fnames[0].find("TEST1_ValidDateOverlap"), std::string::npos);
    TS_ASSERT_EQUALS(fnames.size(), 1);
  }

  void test_nexus_geometry_getInstrumentFilename() {
    const std::string instrumentName = "LOKI";
    InstrumentFileFinder info;
    const auto path = info.getInstrumentFilename(instrumentName, "");
    TS_ASSERT(!path.empty());
    TS_ASSERT(boost::regex_match(path, boost::regex(".*LOKI_Definition\\.hdf5$")));
  }
};
