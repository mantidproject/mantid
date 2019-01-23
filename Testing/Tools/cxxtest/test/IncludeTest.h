// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <cxxtest/TestSuite.h>

//
// This is a test for the --include option
//

class IncludesTest : public CxxTest::TestSuite
{
public:
    void testTraits()
    {
        TS_WARN( (void *)0 );
        TS_WARN( (long *)0 );
    }
};

//
// Local Variables:
// compile-command: "perl test.pl"
// End:
//
