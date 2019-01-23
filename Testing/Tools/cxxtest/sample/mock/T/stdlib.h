// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef __T__STDLIB_H
#define __T__STDLIB_H

#include <stdlib.h>
#include <time.h>

#include <cxxtest/Mock.h>

CXXTEST_MOCK_VOID_GLOBAL( srand, ( unsigned seed ), ( seed ) );
CXXTEST_MOCK_GLOBAL( int, rand, ( void ), () );
CXXTEST_MOCK_GLOBAL( time_t, time, ( time_t *t ), ( t ) );

#endif // __T__STDLIB_H
