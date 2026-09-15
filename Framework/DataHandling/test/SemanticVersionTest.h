// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <stdexcept>

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SemanticVersion.h"

using Mantid::DataHandling::ILLNexus::SemanticVersion;

class SemanticVersionTest : public CxxTest::TestSuite {
public:
  static SemanticVersionTest *createSuite() { return new SemanticVersionTest(); }
  static void destroySuite(SemanticVersionTest *suite) { delete suite; }

  SemanticVersionTest() = default;

  //---------------------------------------------------------------------------------------------
  void testMajor() {

    SemanticVersion version("3");

    TS_ASSERT_EQUALS(version.getMajor(), 3);
    TS_ASSERT_EQUALS(version.getMinor(), 0);
    TS_ASSERT_EQUALS(version.getPatch(), 0);
    TS_ASSERT_EQUALS(version.getPrerelease(), "");
    TS_ASSERT_EQUALS(version.getBuild(), "");
  }

  void testMajorMinor() {

    SemanticVersion version("4.2");

    TS_ASSERT_EQUALS(version.getMajor(), 4);
    TS_ASSERT_EQUALS(version.getMinor(), 2);
    TS_ASSERT_EQUALS(version.getPatch(), 0);
    TS_ASSERT_EQUALS(version.getPrerelease(), "");
    TS_ASSERT_EQUALS(version.getBuild(), "");
  }

  void testMajorMinorPatch() {

    SemanticVersion version("6.3.1");

    TS_ASSERT_EQUALS(version.getMajor(), 6);
    TS_ASSERT_EQUALS(version.getMinor(), 3);
    TS_ASSERT_EQUALS(version.getPatch(), 1);
    TS_ASSERT_EQUALS(version.getPrerelease(), "");
    TS_ASSERT_EQUALS(version.getBuild(), "");
  }

  void testMajorMinorPatchPrerelease() {

    SemanticVersion version("7.0.6-alpha1");

    TS_ASSERT_EQUALS(version.getMajor(), 7);
    TS_ASSERT_EQUALS(version.getMinor(), 0);
    TS_ASSERT_EQUALS(version.getPatch(), 6);
    TS_ASSERT_EQUALS(version.getPrerelease(), "alpha1");
    TS_ASSERT_EQUALS(version.getBuild(), "");
  }

  void testMajorMinorPatchPrereleaseBuild() {

    SemanticVersion version("8.2.4-rc2+commit5367c8");

    TS_ASSERT_EQUALS(version.getMajor(), 8);
    TS_ASSERT_EQUALS(version.getMinor(), 2);
    TS_ASSERT_EQUALS(version.getPatch(), 4);
    TS_ASSERT_EQUALS(version.getPrerelease(), "rc2");
    TS_ASSERT_EQUALS(version.getBuild(), "commit5367c8");
  }

  void testInvalidVersion() {
    // Major which is not an integer
    TS_ASSERT_THROWS(SemanticVersion("x"), const std::runtime_error &);
    // Minor which is not an integer
    TS_ASSERT_THROWS(SemanticVersion("3.x"), const std::runtime_error &);
    // Patch which is not an integer
    TS_ASSERT_THROWS(SemanticVersion("3.2.x"), const std::runtime_error &);

    // Major with negative integer
    TS_ASSERT_THROWS(SemanticVersion("-2"), const std::runtime_error &);
    // Minor with negative integer
    TS_ASSERT_THROWS(SemanticVersion("3.-2"), const std::runtime_error &);
    // Patch with negative integer
    TS_ASSERT_THROWS(SemanticVersion("3.2.-1"), const std::runtime_error &);

    // Invalid prerelease
    TS_ASSERT_THROWS(SemanticVersion("3.2.1rc3"), const std::runtime_error &);
    // Prerelease with spaces
    TS_ASSERT_THROWS(SemanticVersion("3.2.1- rc3 "), const std::runtime_error &);

    // Build with spaces
    TS_ASSERT_THROWS(SemanticVersion("3.2.1-rc2+ commit1234f5"), const std::runtime_error &);
  }

  void testCTOR() {
    SemanticVersion version(4, 5, 7, "rc2", "sha12b4g6");
    TS_ASSERT_EQUALS(version.getMajor(), 4);
    TS_ASSERT_EQUALS(version.getMinor(), 5);
    TS_ASSERT_EQUALS(version.getPatch(), 7);
    TS_ASSERT_EQUALS(version.getPrerelease(), "rc2");
    TS_ASSERT_EQUALS(version.getBuild(), "sha12b4g6");

    version = SemanticVersion(3);
    TS_ASSERT_EQUALS(version.getMajor(), 3);
    TS_ASSERT_EQUALS(version.getMinor(), 0);
    TS_ASSERT_EQUALS(version.getPatch(), 0);
    TS_ASSERT_EQUALS(version.getPrerelease(), "");
    TS_ASSERT_EQUALS(version.getBuild(), "");

    version = SemanticVersion(3, 12);
    TS_ASSERT_EQUALS(version.getMajor(), 3);
    TS_ASSERT_EQUALS(version.getMinor(), 12);
    TS_ASSERT_EQUALS(version.getPatch(), 0);
    TS_ASSERT_EQUALS(version.getPrerelease(), "");
    TS_ASSERT_EQUALS(version.getBuild(), "");

    version = SemanticVersion(3, 11, 3);
    TS_ASSERT_EQUALS(version.getMajor(), 3);
    TS_ASSERT_EQUALS(version.getMinor(), 11);
    TS_ASSERT_EQUALS(version.getPatch(), 3);
    TS_ASSERT_EQUALS(version.getPrerelease(), "");
    TS_ASSERT_EQUALS(version.getBuild(), "");

    version = SemanticVersion(3, 10, 4, "alpha2");
    TS_ASSERT_EQUALS(version.getMajor(), 3);
    TS_ASSERT_EQUALS(version.getMinor(), 10);
    TS_ASSERT_EQUALS(version.getPatch(), 4);
    TS_ASSERT_EQUALS(version.getPrerelease(), "alpha2");
    TS_ASSERT_EQUALS(version.getBuild(), "");

    version = SemanticVersion(3, 10, 4, "alpha2", "c12g4d3");
    TS_ASSERT_EQUALS(version.getMajor(), 3);
    TS_ASSERT_EQUALS(version.getMinor(), 10);
    TS_ASSERT_EQUALS(version.getPatch(), 4);
    TS_ASSERT_EQUALS(version.getPrerelease(), "alpha2");
    TS_ASSERT_EQUALS(version.getBuild(), "c12g4d3");
  }

  void testComparison() {
    SemanticVersion version1("7.0.6");
    SemanticVersion version2("8.1.3");
    TS_ASSERT(version1 < version2);

    version1 = SemanticVersion("8.0.6");
    version2 = SemanticVersion("8.1.3");
    TS_ASSERT(version1 < version2);

    version1 = SemanticVersion("8.1.2");
    version2 = SemanticVersion("8.1.7");
    TS_ASSERT(version1 < version2);

    version1 = SemanticVersion("8.1.2-rc1");
    version2 = SemanticVersion("8.1.7-rc3");
    TS_ASSERT(version1 < version2);

    version1 = SemanticVersion("8.1.7-rc1+sha45f7e6");
    version2 = SemanticVersion("8.1.7-rc1+sha7b87d3");
    TS_ASSERT(version1 == version2);
  }
};
