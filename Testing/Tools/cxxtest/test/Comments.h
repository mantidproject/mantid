// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <cxxtest/TestSuite.h>

//
// This is a test of commenting out tests in CxxTest
//

class Comments : public CxxTest::TestSuite
{
public:
    void test_Something()
    {
        TS_WARN( "Something" );
    }

//     void test_Something_else()
//     {
//         TS_WARN( "Something else" );
//     }

    //void test_Something_else()
    //{
    //    TS_WARN( "Something else" );
    //}
};

//
// Local Variables:
// compile-command: "perl test.pl"
// End:
//
