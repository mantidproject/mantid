// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <cxxtest/TestSuite.h>

//
// This test suite tests double macro invocation
// E.g. when TS_ASSERT_EQUALS( x, y ) fails, it should evaulate x and y once
// Consider TS_ASSERT_EQUALS( readNextValue(), 3 )
//

class DoubleCall : public CxxTest::TestSuite
{
public:
    int i;

    void setUp()
    {
        i = 0;
    }
    
    void testAssertEqualsWithSideEffects()
    {
        TS_ASSERT_EQUALS( increment(), 3 );
    }

    void testAssertDiffersWithSideEffects()
    {
        TS_ASSERT_DIFFERS( increment(), 1 );
    }

    void testAssertDeltaWithSideEffects()
    {
        TS_ASSERT_DELTA( increment(), 2.0, 0.5 );
    }

    int increment()
    {
        return ++i;
    }
};

//
// Local Variables:
// compile-command: "perl test.pl"
// End:
//
