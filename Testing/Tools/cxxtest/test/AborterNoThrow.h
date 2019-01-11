// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <cxxtest/TestSuite.h>

//
// This is a test suite which doesn't use exception handling.
// It is used to verify --abort-on-fail + --have-eh
//

class AborterNoThrow : public CxxTest::TestSuite
{
public:
    void testFailures()
    {
        TS_FAIL(1);
        TS_FAIL(2);
        TS_FAIL(3);
        TS_FAIL(4);
        TS_FAIL(5);
    }
};

//
// Local Variables:
// compile-command: "perl test.pl"
// End:
//
