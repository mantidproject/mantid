// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <cxxtest/TestSuite.h>

class BadTest : public CxxTest::TestSuite
{
public:
    static BadTest *createSuite() { return new BadTest(); }

    void testSomething()
    {
        TS_FAIL( "Bad suite!" );
    }
};

//
// Local Variables:
// compile-command: "perl test.pl"
// End:
//
