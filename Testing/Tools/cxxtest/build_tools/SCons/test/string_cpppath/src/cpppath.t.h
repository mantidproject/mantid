// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/**
 * @file cpppath.t.h
 * This file needs the include in the include dir.
 *
 * @author Gašper Ažman (GA), gasper.azman@gmail.com
 * @version 1.0
 * @since 2008-08-28 11:16:46 AM
 */

// actual path cpppathdir/include.h
#include "include.h"
#include <cxxtest/TestSuite.h>

class CppPathTest : public CxxTest::TestSuite
{
public:
    void test_i_need_me_exists() {
        TS_ASSERT(i_need_me() == 0);
    }
};
