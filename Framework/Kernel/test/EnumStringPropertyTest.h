// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/EnumStringProperty.h"
#include "MantidKernel/EnumeratedString.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;

namespace {
enum class CoolGuys : char { Fred, Joe, Bill, enum_count };
const std::vector<std::string> coolGuyNames{"Frederic", "Joseph", "William"};
enum class Cakes : char { Lemon, Devil, Angel, Bundt, Pound, enum_count };
const std::vector<std::string> cakeNames{"Lemon Cake", "Devil's Food Cake", "Angel Food Fake", "Bundt Cake",
                                         "Pound Cake"};

typedef EnumStringProperty<CoolGuys, &coolGuyNames> COOLGUY_PROPERTY;
typedef EnumStringProperty<Cakes, &cakeNames> CAKE_PROPERTY;
} // namespace

class EnumStringPropertyTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EnumStringPropertyTest *createSuite() { return new EnumStringPropertyTest(); }
  static void destroySuite(EnumStringPropertyTest *suite) { delete suite; }

  void test_Something() { TS_FAIL("You forgot to write a test!"); }
};
