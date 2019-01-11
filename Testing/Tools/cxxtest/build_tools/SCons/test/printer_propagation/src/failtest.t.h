// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef FAILTEST_T_H
#define FAILTEST_T_H

/**
 * @file failtest.t.h
 * This test will succed only with a CrazyRunner.
 *
 * @author
 * @version 1.0
 * @since jue ago 28 14:18:57 ART 2008
 */

#include <cxxtest/TestSuite.h>

class CppPathTest : public CxxTest::TestSuite
{
public:
    void test_i_will_fail() {
        TS_ASSERT(false);
    }
};

#endif
