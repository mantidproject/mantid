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
