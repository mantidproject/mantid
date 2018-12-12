// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <cxxtest/TestSuite.h>

class ThrowNoStd : public CxxTest::TestSuite
{
public:
    void testThrowNoStd()
    {
        TS_ASSERT_THROWS( { throw 1; }, int );
    }
};

//
// Local Variables:
// compile-command: "perl test.pl"
// End:
//
