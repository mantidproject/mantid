#include <cxxtest/TestSuite.h>

//
// A test for issue 122/123, where TS_ASSERT_EQUALS and TS_ASSERT_DIFFERS were
// both true at the same time.
//

// not const on purpose
static char non_const_value_1[] = { "toto" };
static char non_const_value_2[] = { "toto" }; // different pointers from value1
static char non_const_value_3[] = { "nekobus" };
class CharAssertions  : public CxxTest::TestSuite {
  public:
  template <typename T1, typename T2> void differs_must_succeed_generator()
  {
    T1 str1 = non_const_value_1;
    T2 str2 = non_const_value_3;
    TS_ASSERT_DIFFERS(str1, str2);
  }
  template <typename T1, typename T2> void differs_must_fail_generator()
  {
    T1 str1 = non_const_value_1;
    T2 str2 = non_const_value_2;
    TS_ASSERT_DIFFERS(str1, str2);
  }
  template <typename T1, typename T2> void equals_must_succeed_generator()
  {
    T1 str1 = non_const_value_1;
    T2 str2 = non_const_value_2;
    TS_ASSERT_EQUALS(str1, str2);
  }
  template <typename T1, typename T2> void equals_must_fail_generator()
  {
    T1 str1 = non_const_value_1;
    T2 str2 = non_const_value_3;
    TS_ASSERT_EQUALS(str1, str2);
  }

  // we must test the entire matrix
  // naming scheme is test_[de][sf][cm][cm], where:
  // - d or e are for 'differs' and 'equals'
  // - s or f are for 'expect to succeed' or 'expect to fail'
  // - c or m are for 'const' or 'mutable' chars.
  void test_dscc()
  {
    differs_must_succeed_generator<char const* const, char const* const>();
  }
  void test_dscm()
  {
    differs_must_succeed_generator<char const* const, char const*>();
  }
  void test_dsmc()
  {
    differs_must_succeed_generator<char const*, char const* const>();
  }
  void test_dsmm()
  {
    differs_must_succeed_generator<char const*, char const*>();
  }

  void test_dfcc()
  {
    differs_must_fail_generator<char const* const, char const* const>();
  }
  void test_dfcm()
  {
    differs_must_fail_generator<char const* const, char const*>();
  }
  void test_dfmc()
  {
    differs_must_fail_generator<char const*, char const* const>();
  }
  void test_dfmm()
  {
    differs_must_fail_generator<char const*, char const*>();
  }


  void test_escc()
  {
    equals_must_succeed_generator<char const* const, char const* const>();
  }
  void test_escm()
  {
    equals_must_succeed_generator<char const* const, char const*>();
  }
  void test_esmc()
  {
    equals_must_succeed_generator<char const*, char const* const>();
  }
  void test_esmm()
  {
    equals_must_succeed_generator<char const*, char const*>();
  }

  void test_efcc()
  {
    equals_must_fail_generator<char const* const, char const* const>();
  }
  void test_efcm()
  {
    equals_must_fail_generator<char const* const, char const*>();
  }
  void test_efmc()
  {
    equals_must_fail_generator<char const*, char const* const>();
  }
  void test_efmm()
  {
    equals_must_fail_generator<char const*, char const*>();
  }
};
