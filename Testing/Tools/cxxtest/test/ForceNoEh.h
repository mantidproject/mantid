// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <cxxtest/TestSuite.h>

class ForceNoEh : public CxxTest::TestSuite
{
public:
    void testCxxTestCanCompileWithoutExceptionHandling()
    {
        TS_ASSERT_EQUALS( 1, 2 );
        TS_ASSERT_EQUALS( 2, 3 );
        TS_ASSERT_THROWS_NOTHING( foo() );
    }

    void foo()
    {
    }
};

//
// Local Variables:
// compile-command: "perl test.pl"
// End:
//
