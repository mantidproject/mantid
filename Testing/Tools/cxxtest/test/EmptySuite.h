// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <cxxtest/TestSuite.h>

class EmptySuite : public CxxTest::TestSuite
{
public:
    static EmptySuite *createSuite() { return new EmptySuite(); }
    static void destroySuite( EmptySuite *suite ) { delete suite; }

    void setUp() {}
    void tearDown() {}

    void thisSuiteHasNoTests() 
    {
        TS_FAIL( "This suite has no tests" );
    }
};

//
// Local Variables:
// compile-command: "perl test.pl"
// End:
//
