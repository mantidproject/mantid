// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <cxxtest/TestSuite.h>

//
// This test suite is used to test the root/part functionality of CxxTest.
//

class Part1 : public CxxTest::TestSuite
{
public:
    void testSomething()
    {
        TS_ASSERT_THROWS_NOTHING( throwNothing() );
    }

    void throwNothing()
    {
    }
};

//
// Local Variables:
// compile-command: "perl test.pl"
// End:
//
