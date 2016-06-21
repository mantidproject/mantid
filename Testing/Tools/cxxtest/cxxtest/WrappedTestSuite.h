#ifndef WRAPPEDTESTSUITE_H_
#define WRAPPEDTESTSUITE_H_

/**
 * This file is present so that the TestSuite.h header can be included in a precompiled 
 * header file and still have exception handling and the STL
 */

// These must be defined before including a cxxtest header or exception handling is turned off
// The generated cpp file usually takes care of that but we are fording this to be included first
#define _CXXTEST_HAVE_STD
#define _CXXTEST_HAVE_EH
#include <cxxTest/TestSuite.h>
#undef _CXXTEST_LONGLONG // Avoid a warning
#undef _CXXTEST_HAVE_STD
#undef _CXXTEST_HAVE_EH

#endif //WRAPPEDTESTSUITE_H_