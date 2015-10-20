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
