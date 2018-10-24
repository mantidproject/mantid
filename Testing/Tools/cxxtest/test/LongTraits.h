// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//
// This include file is used to test the --include option
//

#include <cxxtest/ValueTraits.h>

namespace CxxTest
{
    CXXTEST_TEMPLATE_INSTANTIATION
    class ValueTraits<long *>
    {
    public:
        ValueTraits( long * ) {}
        const char *asString() { return "(long *)"; }
    };
}

//
// Local Variables:
// compile-command: "perl test.pl"
// End:
//
