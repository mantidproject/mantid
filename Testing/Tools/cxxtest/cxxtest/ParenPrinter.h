// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef __cxxtest__ParenPrinter_h__
#define __cxxtest__ParenPrinter_h__

//
// The ParenPrinter is identical to the ErrorPrinter, except it
// prints the line number in a format expected by some compilers
// (notably, MSVC).
//

#include <cxxtest/ErrorPrinter.h>

namespace CxxTest 
{
    class ParenPrinter : public ErrorPrinter
    {
    public:
        ParenPrinter( CXXTEST_STD(ostream) &o = CXXTEST_STD(cout) ) : ErrorPrinter( o, "(", ")" ) {}
    };
}

#endif // __cxxtest__ParenPrinter_h__
