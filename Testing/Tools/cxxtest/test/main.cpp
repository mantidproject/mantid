// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <cxxtest/TestRunner.h>
#include <cxxtest/TestListener.h>
#include <stdio.h>

//
// This test runner printer some statistics at the end of the run.
// Note that it uses <stdio.h> and not <iostream> for compatibility
// with older compilers.
//

using namespace CxxTest;

class SummaryPrinter : public CxxTest::TestListener
{
public:
    void run()
    {
        CxxTest::TestRunner::runAllTests( *this );
    }
    
    void leaveWorld( const CxxTest::WorldDescription &wd )
    {
        printf( "Number of suites: %u\n", wd.numSuites() );
        printf( "Number of tests: %u\n", wd.numTotalTests() );
        printf( "Number of failed tests: %u\n", TestTracker::tracker().failedTests() );
    }
};

int main()
{
    SummaryPrinter().run();
    return 0;
}

//
// Local Variables:
// compile-command: "perl test.pl"
// End:
//
