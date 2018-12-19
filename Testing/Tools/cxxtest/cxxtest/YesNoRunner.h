// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef __cxxtest__YesNoRunner_h__
#define __cxxtest__YesNoRunner_h__

//
// The YesNoRunner is a simple TestListener that
// just returns true iff all tests passed.
//

#include <cxxtest/TestRunner.h>
#include <cxxtest/TestListener.h>

namespace CxxTest 
{
    class YesNoRunner : public TestListener
    {
    public:
        YesNoRunner()
        {
        }
        
        int run()
        {
            TestRunner::runAllTests( *this );
            return tracker().failedTests();
        }
    };
}

#endif // __cxxtest__YesNoRunner_h__
