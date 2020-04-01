// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/AttenuationProfile.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include <ctime>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;

class AttenuationProfileTest : public CxxTest::TestSuite {
public:
  // This means the constructor isn't called when running other tests
  static AttenuationProfileTest *createSuite() {
    return new AttenuationProfileTest();
  }
  static void destroySuite(AttenuationProfileTest *suite) { delete suite; }

  AttenuationProfileTest() {}

  void testLoadAttenuationFile() {
    std::string path = Mantid::Kernel::ConfigService::Instance().getFullPath(
        "AttenuationProfile.DAT", false, 0);
    TS_ASSERT_THROWS_NOTHING(auto loader = AttenuationProfile(path, ""));
  }

  void testLoadInvalidAttenuationFile() {
    std::string path = Mantid::Kernel::ConfigService::Instance().getFullPath(
        "INVALID.DAT", false, 0);
    TS_ASSERT_THROWS(auto loader = AttenuationProfile(path, ""),
                     Mantid::Kernel::Exception::FileError &);
  }

  void testGetAttenuationCoefficient() {
    std::string path = Mantid::Kernel::ConfigService::Instance().getFullPath(
        "AttenuationProfile.DAT", false, 0);
    auto loader = AttenuationProfile(path, "");
    TS_ASSERT_EQUALS(loader.getAttenuationCoefficient(0.10027009),
                     1000 * 0.82631156E-01);
  }
};
