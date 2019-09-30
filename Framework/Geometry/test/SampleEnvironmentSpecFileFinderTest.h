// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_SAMPLEENVIRONMENTSPECFILEFINDERTEST_H
#define MANTID_GEOMETRY_SAMPLEENVIRONMENTSPECFILEFINDERTEST_H

#include "MantidGeometry/Instrument/SampleEnvironmentFactory.h"
#include <cxxtest/TestSuite.h>

#include <Poco/File.h>
#include <Poco/Path.h>

#include <fstream>
#include <vector>

using Mantid::Geometry::SampleEnvironmentSpecFileFinder;
using Mantid::Geometry::SampleEnvironmentSpec_uptr;

class SampleEnvironmentSpecFileFinderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SampleEnvironmentSpecFileFinderTest *createSuite() {
    return new SampleEnvironmentSpecFileFinderTest();
  }
  static void destroySuite(SampleEnvironmentSpecFileFinderTest *suite) {
    delete suite;
  }

  SampleEnvironmentSpecFileFinderTest() {
    // Setup a temporary directory structure for testing
    Poco::Path testDirec(Poco::Path::temp(),
                         "SampleEnvironmentSpecFileFinderTest");
    testDirec.makeDirectory();
    m_testRoot = testDirec.toString();
    testDirec.append(m_facilityName).append(m_instName);
    Poco::File(testDirec).createDirectories();

    // Write test files
    const std::string xml = "<environmentspec>"
                            " <materials>"
                            "  <material id=\"van\" formula=\"V\"/>"
                            " </materials>"
                            " <components>"
                            "  <containers>"
                            "   <container id=\"10mm\" material=\"van\">"
                            "    <geometry>"
                            "     <sphere id=\"sp-1\">"
                            "      <radius val=\"0.1\"/>"
                            "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                            "     </sphere>"
                            "    </geometry>"
                            "    <samplegeometry>"
                            "     <sphere id=\"sp-1\">"
                            "      <radius val=\"0.1\"/>"
                            "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                            "     </sphere>"
                            "    </samplegeometry>"
                            "   </container>"
                            "  </containers>"
                            " </components>"
                            "</environmentspec>";
    Poco::File envFile(Poco::Path(testDirec, m_envName + ".xml"));
    std::ofstream goodStream(envFile.path(), std::ios_base::out);
    goodStream << xml;
    goodStream.close();
    // Bad file
    envFile = Poco::Path(testDirec, m_badName + ".xml");
    std::ofstream badStream(envFile.path(), std::ios_base::out);
    const std::string wrongContent = "<garbage>";
    badStream << wrongContent;
    badStream.close();
  }

  ~SampleEnvironmentSpecFileFinderTest() {
    try {
      Poco::File(m_testRoot).remove(true);
    } catch (...) {
    }
  }

  //----------------------------------------------------------------------------
  // Success tests
  //----------------------------------------------------------------------------
  void test_Finder_Returns_Correct_Spec_If_Exists() {
    SampleEnvironmentSpecFileFinder finder(
        std::vector<std::string>(1, m_testRoot));

    SampleEnvironmentSpec_uptr spec;
    TS_ASSERT_THROWS_NOTHING(
        spec = finder.find(m_facilityName, m_instName, m_envName));

    // Does it look right
    TS_ASSERT_EQUALS(m_envName, spec->name());
    TS_ASSERT_EQUALS(1, spec->ncans());
  }

  //----------------------------------------------------------------------------
  // Failure tests
  //---------------------------------------------------------------------------
  void test_Finder_Throws_If_Empty_Directory_List_Given() {
    std::vector<std::string> empty;
    TS_ASSERT_THROWS(SampleEnvironmentSpecFileFinder finder(empty),
                     const std::invalid_argument &);
  }

  void test_Finder_Throws_If_Facility_Correct_Instrument_Incorrect() {
    SampleEnvironmentSpecFileFinder finder(
        std::vector<std::string>(1, m_testRoot));
    TS_ASSERT_THROWS(finder.find(m_facilityName, "unknown", m_envName),
                     const std::runtime_error &);
  }

  void test_Finder_Throws_If_Facility_Incorrect_Instrument_Correct() {
    SampleEnvironmentSpecFileFinder finder(
        std::vector<std::string>(1, m_testRoot));
    TS_ASSERT_THROWS(finder.find("unknown", m_instName, m_envName),
                     const std::runtime_error &);
  }

  void test_Finder_Throws_If_Facility_Instrument_Correct_Bad_Environment() {
    SampleEnvironmentSpecFileFinder finder(
        std::vector<std::string>(1, m_testRoot));
    TS_ASSERT_THROWS(finder.find(m_facilityName, m_instName, "unknown"),
                     const std::runtime_error &);
  }

  void test_Finder_Throws_If_Filename_Found_But_Content_Invalid() {
    SampleEnvironmentSpecFileFinder finder(
        std::vector<std::string>(1, m_testRoot));
    TS_ASSERT_THROWS(finder.find(m_facilityName, m_instName, m_badName),
                     const std::runtime_error &);
  }

private:
  std::string m_testRoot;
  const std::string m_facilityName = "TestingFacility";
  const std::string m_instName = "TestingInst";
  const std::string m_envName = "TestingEnv";
  const std::string m_badName = "BadEnv";
};

#endif // MANTID_GEOMETRY_SAMPLEENVIRONMENTSPECFILEFINDERTEST_H
