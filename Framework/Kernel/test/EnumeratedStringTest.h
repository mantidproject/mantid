// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/EnumeratedString.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;

namespace {
enum class CoolGuys { Fred, Joe, Bill, enum_count };
static std::string coolGuyNames[] = {"Frederic", "Joseph", "William"};
enum class Cakes { Lemon, Devil, Angel, Bundt, Pound, enum_count };
static std::string cakeNames[] = {"Lemon Cake", "Devil's Food Cake", "Angel Food Fake", "Bundt Cake", "Pound Cake"};
typedef EnumeratedString<CoolGuys, coolGuyNames> COOLGUY;
typedef EnumeratedString<Cakes, cakeNames> CAKE;
} // namespace

class EnumeratedStringTest : public CxxTest::TestSuite {
private:
public:
  void testConstructor() {
    // test constructor from enumerator
    COOLGUY dude1(CoolGuys::Fred);
    TS_ASSERT_EQUALS(dude1, CoolGuys(0));
    TS_ASSERT_EQUALS(dude1, CoolGuys::Fred);
    TS_ASSERT_EQUALS(dude1, coolGuyNames[0]);
    TS_ASSERT_DIFFERS(dude1, CoolGuys::Bill);

    // test constructor from string name
    COOLGUY dude2(coolGuyNames[1]);
    TS_ASSERT_EQUALS(dude2, CoolGuys(1));
    TS_ASSERT_EQUALS(dude2, CoolGuys::Joe);
    TS_ASSERT_EQUALS(dude2, coolGuyNames[1]);
    TS_ASSERT_DIFFERS(dude2, CoolGuys::Bill);

    TS_ASSERT_DIFFERS(dude1, dude2);
    TS_ASSERT_THROWS_NOTHING(dude1 = "Joseph"); // Joe is a cool guy
    TS_ASSERT_EQUALS(dude1, dude2);

    // test copy constructor from enumerated string
    COOLGUY dude3(dude1);
    TS_ASSERT_EQUALS(dude3, dude1);

    // test constructor from strng literal
    COOLGUY dude4("William");
    TS_ASSERT_EQUALS(dude4, CoolGuys::Bill)
  }

  void testBadConstructor() {
    // test failure if initializing from bad string or bad enum
    TS_ASSERT_THROWS_ANYTHING(COOLGUY dude1("Jeremy"););
    TS_ASSERT_THROWS_ANYTHING(COOLGUY dude2(CoolGuys(-1)););
    TS_ASSERT_THROWS_ANYTHING(COOLGUY dude3(CoolGuys(5)););
  }

  void testAssignment() {
    COOLGUY dude;
    std::vector<COOLGUY> coolGuys;
    // make a vector of all CoolGuy values
    for (CoolGuys e = CoolGuys(0); size_t(e) < size_t(CoolGuys::enum_count); e = CoolGuys(size_t(e) + 1))
      TS_ASSERT_THROWS_NOTHING(coolGuys.push_back(COOLGUY(e)));
    // assign from enumerator
    for (size_t i = 0; i < size_t(CoolGuys::enum_count); i++) {
      TS_ASSERT_THROWS_NOTHING(dude = CoolGuys(i));
      TS_ASSERT(dude == CoolGuys(i));
      TS_ASSERT(dude == coolGuys[i]);
      TS_ASSERT(dude == coolGuyNames[i]);
    }
    // assign from string name
    for (size_t i = 0; i < size_t(CoolGuys::enum_count); i++) {
      TS_ASSERT_THROWS_NOTHING(dude = coolGuyNames[i]);
      TS_ASSERT(dude == CoolGuys(i));
      TS_ASSERT(dude == coolGuys[i]);
      TS_ASSERT(dude == coolGuyNames[i]);
    }

    // must also check inequality comparator explicitly,
    // since TS runs !(x==y) instead of x!=y
    dude = CoolGuys::Fred;
    TS_ASSERT(dude == CoolGuys::Fred);
    TS_ASSERT(dude != CoolGuys::Bill);
    TS_ASSERT(dude == coolGuyNames[0]);
    TS_ASSERT(dude == "Frederic");
    TS_ASSERT(dude != coolGuys[2]);
    TS_ASSERT(dude != "William");

    // test assignment from other enumerated string
    CAKE cake1 = Cakes::Angel;
    CAKE cake2 = cake1;
    TS_ASSERT_EQUALS(cake1, cake2);

    // assign from string literal, which is also a name
    dude = "Frederic";
    TS_ASSERT_EQUALS(dude, CoolGuys::Fred);
  }

  void testBadAssignment() {
    CAKE cake = Cakes(3);
    COOLGUY dude("William");
    TS_ASSERT_THROWS_ANYTHING(dude = "Jeremy"); // Jeremy is not a cool guy
    TS_ASSERT_DIFFERS(dude, "Jeremy");          // make sure nothing was set
    TS_ASSERT_EQUALS(dude, "William");          // let's stick to Bill
    TS_ASSERT_EQUALS(dude, CoolGuys::Bill);
    // make sure assigning to enum_count fails
    TS_ASSERT_THROWS_ANYTHING(cake = Cakes::enum_count);
    TS_ASSERT_THROWS_ANYTHING(dude = CoolGuys::enum_count);
    // make sure assigning to spurious enumerators fails
    TS_ASSERT_THROWS_ANYTHING(dude = CoolGuys(5););
    TS_ASSERT_THROWS_ANYTHING(cake = Cakes(-1););
  }

  // test ability to case from enumerated string to other objects
  void testCasting() {
    CAKE cake(Cakes::Pound);

    // test we can cast from enumerated string to enum
    Cakes poundCake = cake;
    TS_ASSERT_EQUALS(poundCake, Cakes::Pound);

    // test we can cast frum enumerated string to string
    std::string poundQueque = cake;
    TS_ASSERT_EQUALS(poundQueque, "Pound Cake");

    // check ability to cast to numeric types
    for (size_t i = 0; i < size_t(Cakes::enum_count); i++) {
      cake = cakeNames[i];   // init with string for extra testy
      Cakes cakeEnum = cake; // cast to enum
      TS_ASSERT_EQUALS(cakeEnum, Cakes(i));
      // check that when cast to different numric types, is equal
      TS_ASSERT_EQUALS(size_t(cakeEnum), size_t(i));
      TS_ASSERT_EQUALS(int(cakeEnum), int(i));
      TS_ASSERT_EQUALS(char(cakeEnum), char(i));
      TS_ASSERT_EQUALS(double(cakeEnum), double(i));
    }
    // also check string conversions
    for (size_t i = 0; i < size_t(Cakes::enum_count); i++) {
      cake = Cakes(i);             // init with enum for extra testy
      std::string cakeName = cake; // cast to string
      // verify equality
      TS_ASSERT_EQUALS(cakeName, cakeNames[i]);
      TS_ASSERT_EQUALS(std::string(cake), cakeNames[i]);
    }
    // check ability cast to enum, string
    for (size_t i = 0; i < size_t(Cakes::enum_count); i++) {
      Cakes cakeEnum1 = Cakes(i);
      cake = cakeEnum1;
      Cakes cakeEnum2 = cake;
      std::string cakeName = cake;
      TS_ASSERT_EQUALS(cakeEnum1, cakeEnum2);
      TS_ASSERT_EQUALS(cakeName, cakeNames[i]);
    }
  }

  /*****
   * For testing enumerated strings as arguments to functions
   */
  bool functionOfCake(const CAKE &tasty) { return (tasty == Cakes(0)); }

  bool functionOfEnum(const Cakes &tastyType) { return (tastyType == Cakes(0)); }

  bool functionOfString(const std::string &tastyName) { return (tastyName == cakeNames[0]); }

  void testAsFunctionArg() {
    // test that enumerated string parameter can be set by enum or string
    CAKE scrumptious = Cakes(0);
    TS_ASSERT(functionOfCake(scrumptious));
    TS_ASSERT(functionOfCake(Cakes(0)));
    TS_ASSERT(functionOfCake(cakeNames[0]))
    scrumptious = Cakes(1);
    TS_ASSERT(!functionOfCake(scrumptious));
    TS_ASSERT(!functionOfCake(Cakes(1)));
    TS_ASSERT(!functionOfCake(cakeNames[1]))

    // test that enum parameter can be set by enumerated string
    TS_ASSERT(!functionOfEnum(scrumptious));
    scrumptious = Cakes(0);
    TS_ASSERT(functionOfEnum(scrumptious));
    // test that string parameter can be set by enumerated string
    TS_ASSERT(functionOfString(scrumptious));
    TS_ASSERT(functionOfString("Lemon Cake"))
    scrumptious = Cakes(3);
    TS_ASSERT(!functionOfString(scrumptious));
    TS_ASSERT(!functionOfString("Bundt Cake"));
  }

  void testEnumCount() {
    // try removing enum_count -- should give a compiler error
    enum class Letters { a, b, enum_count };
    static std::string letters[2] = {"a", "b"};
    enum Graphia { alpha, beta, enum_count };
    static std::string graphia[2] = {"alpha", "beta"};
    EnumeratedString<Letters, letters> l1("a");
    EnumeratedString<Graphia, graphia> l2("alpha");
    TS_ASSERT_DIFFERS(l1, l2);
  }

  void testSwitchAndIf() {
    const size_t index = 3;
    CAKE tasty = Cakes(index), scrumptious = Cakes(index);

    // test enumerated string against string
    if (tasty == cakeNames[index]) {
    } else
      TS_FAIL("EnumeratedString in 'IF' failed to compare against string name");

    // test enumerated string against enum
    if (tasty == Cakes(index)) {
    } else
      TS_FAIL("EnumeratedString in 'IF' failed to compare against enumerated value");

    // test switch on enumerated string, enum cases
    switch (tasty) {
    case Cakes(index):
      break;
    default:
      TS_FAIL("EnumeratedString in 'SWITCH' failed to match to enumerated value");
    }

    // test switch on enumerated string, enum cases written out
    switch (tasty) {
    case Cakes::Bundt: // Cakes(3) is a Bundt cake
      break;
    default:
      TS_FAIL("EnumeratedString in 'SWITCH' failed to match to enumerated value");
    }
  }
};
