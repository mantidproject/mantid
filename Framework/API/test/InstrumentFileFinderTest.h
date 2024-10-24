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

#include <Poco/DirectoryIterator.h>
#include <Poco/TemporaryFile.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <cxxtest/TestSuite.h>

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
    Poco::DirectoryIterator end_iter;
    for (Poco::DirectoryIterator dir_itr(ConfigService::Instance().getString("instrumentDefinition.directory"));
         dir_itr != end_iter; ++dir_itr) {
      if (!Poco::File(dir_itr->path()).isFile())
        continue;

      std::string l_filenamePart = Poco::Path(dir_itr->path()).getFileName();

      if (boost::regex_match(l_filenamePart, regex)) {
        std::string validFrom, validTo;
        InstrumentFileFinder::getValidFromTo(dir_itr->path(), validFrom, validTo);

        size_t found;
        found = l_filenamePart.find("_Definition");
        fromToEntry ft;
        ft.path = dir_itr->path();
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
    const auto tmpDir = Poco::Path::temp();
    const std::string filename = "test_Parameters.xml";
    const std::string expectedPath = tmpDir + filename;

    Poco::TemporaryFile::registerForDeletion(expectedPath);
    Poco::File fileHandle(expectedPath);
    fileHandle.createFile();

    const auto result = InstrumentFileFinder::getParameterPath("test", tmpDir);
    // Ensure file was found and it's in the tmp dir
    TS_ASSERT(result.find(filename) != std::string::npos);
    TS_ASSERT(result.find(tmpDir) != std::string::npos);
  }

  void testNonExistantIPFWithHint() {
    const auto tmpDir = Poco::Path::temp();
    const auto result = InstrumentFileFinder::getParameterPath("notThere", tmpDir);
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
