// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/EnumeratedString.h"
#include <cxxtest/TestSuite.h>
#include <json/value.h>

using namespace Mantid::Kernel;

namespace {
enum class CoolGuys { Fred, Joe, Bill, enum_count };
static std::string coolGuyNames[] = {"Frederic", "Joseph", "William"};
enum class Cakes { Lemon, Devil, Angel, Bundt, Pound, enum_count };
static std::string cakeNames[] = {"Lemon Cake", "Devil's Food Cake", "Angel Food Fake", "Bundt Cake", "Pound Cake"};
} // namespace

class EnumeratedStringTest : public CxxTest::TestSuite {

private:
public:
  void testConstructor() {
    EnumeratedString<CoolGuys, coolGuyNames> dude1(CoolGuys::Fred);
    EnumeratedString<CoolGuys, coolGuyNames> dude2(coolGuyNames[1]);
    TS_ASSERT_EQUALS(dude1, CoolGuys(0));
    TS_ASSERT_EQUALS(dude1, CoolGuys::Fred);
    TS_ASSERT_EQUALS(dude1, coolGuyNames[0]);
    TS_ASSERT_EQUALS(dude2, CoolGuys(1));
    TS_ASSERT_EQUALS(dude2, CoolGuys::Joe);
    TS_ASSERT_EQUALS(dude2, coolGuyNames[1]);

    TS_ASSERT_DIFFERS(dude1, dude2);
    TS_ASSERT_THROWS_NOTHING(dude1 = "Joseph"); // Joe is a cool guy
    TS_ASSERT_EQUALS(dude1, dude2);
    TS_ASSERT_THROWS_ANYTHING(dude1 = "Jeremy"); // Jeremy is not a cool guy
    TS_ASSERT_DIFFERS(dude1, "Jeremy");
    TS_ASSERT_EQUALS(dude1, CoolGuys::Joe); // let's stick to Joe
    TS_ASSERT_EQUALS(dude1, coolGuyNames[1]);
  }

  void testBadConstructor() {
    typedef EnumeratedString<CoolGuys, coolGuyNames> COOLGUY;
    TS_ASSERT_THROWS_ANYTHING(COOLGUY dude("Jeremy"););
    TS_ASSERT_THROWS_ANYTHING(use(CoolGuys(4)););
  }

  void testEnumCount() {
    EnumeratedString<Cakes, cakeNames> cake;
    EnumeratedString<CoolGuys, coolGuyNames> dude;

    TS_ASSERT_THROWS_ANYTHING(cake = Cakes::enum_count);
    TS_ASSERT_THROWS_ANYTHING(dude = CoolGuys::enum_count);
    TS_ASSERT_THROWS_ANYTHING(dude = "Jeremy");         // Jeremy is not a cool guy
    TS_ASSERT_THROWS_ANYTHING(cake = "French Vanilla"); // french vanilla is not a cake

    enum DifferentCakes { Lava, Cobbler, enum_count };
    static std::string differentCakes[2] = {"Lava Cake", "Cobbler (not a cake)"};
    EnumeratedString<DifferentCakes, differentCakes> differentCake;
    TS_ASSERT_THROWS_NOTHING(differentCake = DifferentCakes::Lava);
    TS_ASSERT_DIFFERS(cake, differentCake);
  }

  void testChangesToValues() {
    EnumeratedString<CoolGuys, coolGuyNames> dude;
    std::vector<CoolGuys> coolGuys;
    for (CoolGuys e = CoolGuys(0); size_t(e) < size_t(CoolGuys::enum_count); e = CoolGuys(size_t(e) + 1))
      TS_ASSERT_THROWS_NOTHING(coolGuys.push_back(e));
    for (size_t i = 0; i < size_t(CoolGuys::enum_count); i++) {
      TS_ASSERT_THROWS_NOTHING(dude = CoolGuys(i));
      TS_ASSERT(dude == CoolGuys(i));
      TS_ASSERT(dude == coolGuys[i]);
      TS_ASSERT(dude == coolGuyNames[i]);
    }
  }

  void testSwitchAndIf() {
    const size_t index = 3;
    EnumeratedString<Cakes, cakeNames> tasty = Cakes(index);

    if (tasty == cakeNames[index]) {
    } else
      TS_FAIL("EnumeratedString in if failed to compare against string name");

    if (tasty == Cakes(index)) {
    } else
      TS_FAIL("EnumeratedString in if failed to compare against enumerated value");

    switch (tasty) {
    case Cakes(index):
      break;
    default:
      TS_FAIL("EnumeratedString in switch failed to match to enumerated value");
    }

    const Cakes secondHelping = Cakes(index);

    switch (tasty) {
    case secondHelping:
      break;
    default:
      TS_FAIL("EnumeratedString in switch failed to match to enumerated value");
    }
    switch (tasty) {
    case Cakes::Bundt: // Cakes(3) is a Bundt cake
      break;
    default:
      TS_FAIL("EnumeratedString in switch failed to match to enumerated value");
    }
  }

  void testCopyConstructor() {
    EnumeratedString<Cakes, cakeNames> cake(Cakes::Lemon);
    Cakes kk = (Cakes)cake;
    std::string ks = (std::string)cake;
    TS_ASSERT_EQUALS(kk, Cakes::Lemon);
    TS_ASSERT_EQUALS(ks, cakeNames[size_t(kk)]);

    Cakes kt = kk;
    TS_ASSERT_EQUALS(kt, kk);

    EnumeratedString<Cakes, cakeNames> cakey = cake;
    TS_ASSERT_EQUALS(cakey, cake);
  }

  void testCasting() {
    EnumeratedString<Cakes, cakeNames> cake;
    // check ability to cast to numeric types
    for (size_t i = 0; i < size_t(Cakes::enum_count); i++) {
      cake = cakeNames[i]; // init with string for extra testy
      TS_ASSERT_EQUALS(size_t((Cakes)cake), size_t(i));
      TS_ASSERT_EQUALS(int((Cakes)cake), int(i));
      TS_ASSERT_EQUALS(char((Cakes)cake), char(i));
      TS_ASSERT_EQUALS(double((Cakes)cake), double(i));
    }
    // also check string conversions
    for (size_t i = 0; i < size_t(Cakes::enum_count); i++) {
      cake = Cakes(i); // init with enum for extra tesy
      TS_ASSERT((std::string(cake) == cakeNames[i]));
    }
    // check ability copy construct to enum, string
    for (size_t i = 0; i < size_t(Cakes::enum_count); i++) {
      cake = Cakes(i);
      Cakes cakeType = cake;
      std::string cakeName = cake;
      TS_ASSERT_EQUALS(cake, cakeType);
      TS_ASSERT_EQUALS(Cakes(i), cakeType);
      TS_ASSERT_EQUALS(cakeName, cakeName);
    }
  }
};
