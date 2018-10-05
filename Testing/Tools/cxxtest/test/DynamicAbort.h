// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <cxxtest/TestSuite.h>

class DynamicAbort : public CxxTest::TestSuite
{
public:
    void test_Abort_on_fail_in_this_test()
    {
        TS_ASSERT_EQUALS( 1, 2 );
        TS_ASSERT_EQUALS( 2, 3 );
    }

    void test_Dont_abort_in_this_test()
    {
        CxxTest::setAbortTestOnFail( false );
        TS_ASSERT_EQUALS( 1, 2 );
        TS_ASSERT_EQUALS( 2, 3 );
    }

    void test_Revert_to_abort()
    {
        TS_ASSERT_EQUALS( 1, 2 );
        TS_ASSERT_EQUALS( 2, 3 );
    }
};

class SetUpWorksAllTests : public CxxTest::TestSuite
{
public:
    void setUp()
    {
        CxxTest::setAbortTestOnFail( false );
    }

    void test_Dont_abort_in_this_test()
    {
        TS_ASSERT_EQUALS( 1, 2 );
        TS_ASSERT_EQUALS( 2, 3 );
    }

    void test_Dont_abort_in_this_test_either()
    {
        TS_ASSERT_EQUALS( 1, 2 );
        TS_ASSERT_EQUALS( 2, 3 );
    }

    void test_Override_in_this_test()
    {
        CxxTest::setAbortTestOnFail( true );
        TS_ASSERT_EQUALS( 1, 2 );
        TS_ASSERT_EQUALS( 2, 3 );
    }
};

//
// Local Variables:
// compile-command: "perl test.pl"
// End:
//
