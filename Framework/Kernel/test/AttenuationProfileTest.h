// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source,
//     Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/AttenuationProfile.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Material.h"
#include <ctime>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;

class AttenuationProfileTest : public CxxTest::TestSuite {
public:
  // This means the constructor isn't called when running other tests
  static AttenuationProfileTest *createSuite() { return new AttenuationProfileTest(); }
  static void destroySuite(AttenuationProfileTest *suite) { delete suite; }

  AttenuationProfileTest() {}

  void testLoadAttenuationFile() {
    std::string path = Mantid::Kernel::ConfigService::Instance().getFullPath("AttenuationProfile.DAT", false, 0);
    TS_ASSERT(!path.empty());
    auto profile = AttenuationProfile(path, "");
    TS_ASSERT_THROWS_NOTHING(auto profile = AttenuationProfile(path, ""));
  }

  void testLoadInvalidAttenuationFile() {
    std::string path = Mantid::Kernel::ConfigService::Instance().getFullPath("INVALID.DAT", false, 0);
    TS_ASSERT_THROWS(auto profile = AttenuationProfile(path, ""), Mantid::Kernel::Exception::FileError &);
  }

  void testGetAttenuationCoefficient() {
    std::string path = Mantid::Kernel::ConfigService::Instance().getFullPath("AttenuationProfile.DAT", false, 0);
    auto profile = AttenuationProfile(path, "");
    TS_ASSERT_EQUALS(profile.getAttenuationCoefficient(0.10027009), 1000 * 0.82631156E-01);
  }

  void testGetAttenuationCoefficientBeyondRange() {
    Material::ChemicalFormula formula;
    formula = Material::parseChemicalFormula("C");
    auto testMaterial = std::make_unique<Material>("test", formula, 3.51); // diamond
    std::string path = Mantid::Kernel::ConfigService::Instance().getFullPath("AttenuationProfile.DAT", false, 0);
    auto profile = AttenuationProfile(path, "", testMaterial.get());
    auto profileWithEmptyMaterial = AttenuationProfile(path, "");
    TS_ASSERT_EQUALS(profile.getAttenuationCoefficient(0), testMaterial->attenuationCoefficient(0));
    // double check the supplied attenuation profile didn't happen to have
    // a value at zero matching the coefficient from the material
    TS_ASSERT_DIFFERS(profileWithEmptyMaterial.getAttenuationCoefficient(0), testMaterial->attenuationCoefficient(0));
  }
};
