// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/Logger.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;

namespace {
/// static Logger definition
Logger g_log("EnumeratedStringTest");
} // namespace

namespace {
enum class CoolGuys : char { Fred, Joe, Bill, enum_count };
const std::vector<std::string> coolGuyNames{"Frederic", "Joseph", "William"};
enum class Cakes : char { Lemon, Devil, Angel, Bundt, Pound, enum_count };
const std::vector<std::string> cakeNames{"Lemon Cake", "Devil's Food Cake", "Angel Food Fake", "Bundt Cake",
                                         "Pound Cake"};
typedef EnumeratedString<CoolGuys, &coolGuyNames> COOLGUY;
typedef EnumeratedString<Cakes, &cakeNames> CAKE;
} // namespace

class EnumeratedStringTest : public CxxTest::TestSuite {
private:
public:
  void testConstructor() {
    g_log.notice("\ntestConstructor...");

    // test constructor from enumerator
    COOLGUY dude1(CoolGuys::Fred);
    TS_ASSERT_EQUALS(dude1, "Frederic");
    TS_ASSERT_EQUALS(dude1, CoolGuys(0));
    TS_ASSERT_EQUALS(dude1, CoolGuys::Fred);
    TS_ASSERT_EQUALS(dude1, coolGuyNames[0]);
    TS_ASSERT_DIFFERS(dude1, CoolGuys::Bill);
    TS_ASSERT_DIFFERS(dude1, "Jeremy");

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
    g_log.notice("\ntestAssignment...");

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
    g_log.notice("\ntestBadAssignment...");

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
    g_log.notice("\ntestCasting...");

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
    g_log.notice("\ntestAsFunctionArg...");

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
    g_log.notice("\ntestEnumCount...");

    // try removing enum_count -- should give a compiler error
    enum class Letters : size_t { a, b, enum_count };
    static const std::vector<std::string> letters{"a", "b"};
    enum class Graphia : size_t { alpha, beta, enum_count };
    static const std::vector<std::string> graphia{"alpha", "beta"};
    EnumeratedString<Letters, &letters> l1("a");
    EnumeratedString<Graphia, &graphia> l2("alpha");
    TS_ASSERT_DIFFERS(l1, l2);
  }

  void testFailIfWrongNumbers() {
    g_log.notice("\ntestFailIfWrongNumbers...");

    enum Letters : size_t { a, b, c, enum_count };
    static const std::vector<std::string> letters = {"a", "b"};
    typedef EnumeratedString<Letters, &letters> LETTERS;
    TS_ASSERT_THROWS_ANYTHING(LETTERS l1);
    TS_ASSERT_THROWS_ANYTHING(LETTERS l2(Letters::a));
    TS_ASSERT_THROWS_ANYTHING(LETTERS l1("a"));
  }

  void testSwitchAndIf() {
    g_log.notice("\ntestSwitchAndIf...");

    const char index = 3;
    CAKE tasty = Cakes(index);

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

  void testUnderlyingType() {
    g_log.notice("\ntestUnderlyingType...");

    // use a non-class enum to compare against chars
    enum LetterA : char { a, enum_count };
    static const std::vector<std::string> letterA = {"a"};
    typedef EnumeratedString<LetterA, &letterA> LETTERA;
    LETTERA a1 = a;                // note that a is in namespace, vars cannot be named this
    TS_ASSERT_EQUALS(a1, a);       // compare against enum
    TS_ASSERT_EQUALS(a1, "a");     // compares against string
    TS_ASSERT_EQUALS(a1, 0);       // compare against literal
    TS_ASSERT_EQUALS(a1, '\x00');  // char literal at 0
    TS_ASSERT_DIFFERS(a1, 'a');    // Letters::a corresponds to 0, not to 'a'= 97
    TS_ASSERT_DIFFERS(a1, '\x01'); // char literal at 1
    TS_ASSERT_DIFFERS(a1, 1);
    switch (a1) {
    case '\x00':
      break;
    default:
      TS_FAIL("EnumeratedString in 'SWITCH' failed to match to underlying type");
    }
  }

  void testCaseInsensitiveNameComparison() {
    g_log.notice("\ntestCaseInsensitiveNameComparison...");

    enum TwoLettersEnum : size_t { ab, cd, enum_count };
    static const std::vector<std::string> twoLetters = {"ab", "cd"};
    typedef EnumeratedString<TwoLettersEnum, &twoLetters, &compareStringsCaseInsensitive> TWO_LETTERS;

    // 1. Test a use case with mixed-case string introduced through the constructor: EnumeratedString(const std::string
    // s)
    TS_ASSERT_THROWS_NOTHING(TWO_LETTERS enTwoLetters("aB"));
    TWO_LETTERS enTwoLetters_from_constructor("aB");
    TS_ASSERT(enTwoLetters_from_constructor.c_str() == std::string("aB"))
    TS_ASSERT(enTwoLetters_from_constructor == TwoLettersEnum::ab)

    // test operator==(const char *s) const
    TS_ASSERT(enTwoLetters_from_constructor == "ab");
    TS_ASSERT(enTwoLetters_from_constructor == "aB");
    TS_ASSERT(enTwoLetters_from_constructor == "Ab");
    TS_ASSERT(enTwoLetters_from_constructor == "AB");

    // test operator==(const std::string& s) const
    TS_ASSERT(enTwoLetters_from_constructor == std::string("ab"));
    TS_ASSERT(enTwoLetters_from_constructor == std::string("aB"));
    TS_ASSERT(enTwoLetters_from_constructor == std::string("Ab"));
    TS_ASSERT(enTwoLetters_from_constructor == std::string("AB"));

    TS_ASSERT(!(enTwoLetters_from_constructor == "cd"));
    TS_ASSERT(!(enTwoLetters_from_constructor == "bA"));
    TS_ASSERT(!(enTwoLetters_from_constructor == std::string("cd")));
    TS_ASSERT(!(enTwoLetters_from_constructor == std::string("bA")));

    // test operator!=(const char *s) const
    TS_ASSERT(enTwoLetters_from_constructor != "cd");
    TS_ASSERT(enTwoLetters_from_constructor != "Ba");

    // test operator!=(const std::string& s)
    TS_ASSERT(enTwoLetters_from_constructor != std::string("cd"));
    TS_ASSERT(enTwoLetters_from_constructor != std::string("BA"));

    // 2. Test a use case with mixed-case string introduced through the assignment operator: EnumeratedString
    // &operator=(const std::string& s)
    TS_ASSERT_THROWS_NOTHING(TWO_LETTERS enTwoLetters_from_assignment = std::string("aB"));
    TWO_LETTERS enTwoLetters_from_assignment = std::string("aB");
    TS_ASSERT(enTwoLetters_from_assignment.c_str() == std::string("aB"))
    TS_ASSERT(enTwoLetters_from_assignment == TwoLettersEnum::ab)

    // test operator==(const char *s) const
    TS_ASSERT(enTwoLetters_from_assignment == "ab");
    TS_ASSERT(enTwoLetters_from_assignment == "aB");
    TS_ASSERT(enTwoLetters_from_assignment == "Ab");
    TS_ASSERT(enTwoLetters_from_assignment == "AB");

    // test operator==(const std::string& s) const
    TS_ASSERT(enTwoLetters_from_assignment == std::string("ab"));
    TS_ASSERT(enTwoLetters_from_assignment == std::string("aB"));
    TS_ASSERT(enTwoLetters_from_assignment == std::string("Ab"));
    TS_ASSERT(enTwoLetters_from_assignment == std::string("AB"));

    TS_ASSERT(!(enTwoLetters_from_assignment == "cd"));
    TS_ASSERT(!(enTwoLetters_from_assignment == "bA"));
    TS_ASSERT(!(enTwoLetters_from_assignment == std::string("cd")));

    // test operator!=(const char *s) const
    TS_ASSERT(enTwoLetters_from_assignment != "cd");
    TS_ASSERT(enTwoLetters_from_assignment != "Ba");

    // test operator!=(const std::string& s)
    TS_ASSERT(enTwoLetters_from_assignment != std::string("cd"));
    TS_ASSERT(enTwoLetters_from_assignment != std::string("BA"));
  }

  void test_customNameComparator() {
    g_log.notice("\ntest_customNameComparator...");

    enum FirstLetterEnum : size_t { A, B, C, enum_count };
    static const std::vector<std::string> words = {"apple", "banana", "cherry"};
    static std::function<bool(const std::string &, const std::string &)> firstLetterComparator =
        [](const std::string &x, const std::string &y) { return x[0] == y[0]; };
    typedef EnumeratedString<FirstLetterEnum, &words, &firstLetterComparator> Fruit;

    Fruit apple = FirstLetterEnum::A;
    Fruit banana = FirstLetterEnum::B;
    Fruit cherry = FirstLetterEnum::C;

    TS_ASSERT_EQUALS(apple, "apricot");
    TS_ASSERT_EQUALS(banana, "blueberry");
    TS_ASSERT_EQUALS(cherry, "corn");
  }
};
