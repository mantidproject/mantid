// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <cxxtest/TestSuite.h>

//
// This test suite demonstrates TS_ASSERT_SAME_DATA
//

class SameData : public CxxTest::TestSuite
{
public:
    enum { DATA_SIZE = 24 };
    unsigned char x[DATA_SIZE], y[DATA_SIZE];

    void setUp()
    {
        for ( unsigned i = 0; i < DATA_SIZE; ++ i ) {
            x[i] = (unsigned char)i;
            y[i] = (unsigned char)~x[i];
        }
    }

    void testAssertSameData()
    {
        TS_ASSERT_SAME_DATA( x, y, DATA_SIZE );
    }

    void testAssertMessageSameData()
    {
        TSM_ASSERT_SAME_DATA( "Not same data", x, y, DATA_SIZE );
    }

    void testSafeAssertSameData()
    {
        ETS_ASSERT_SAME_DATA( x, y, DATA_SIZE );
    }

    void testSafeAssertMessageSameData()
    {
        ETSM_ASSERT_SAME_DATA( "Not same data", x, y, DATA_SIZE );
    }
};

//
// Local Variables:
// compile-command: "perl test.pl"
// End:
//
