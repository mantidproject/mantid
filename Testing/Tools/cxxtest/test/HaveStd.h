// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <cxxtest/TestSuite.h>

//
// This tests CxxTest's `--have-std' option
//
#include "Something.h"

class HaveStd : public CxxTest::TestSuite
{
public:
    void testHaveStd()
    {
        TS_ASSERT_EQUALS( something(), "Something" );
    }
};

//
// Local Variables:
// compile-command: "perl test.pl"
// End:
//
