// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef __DELTATEST_H
#define __DELTATEST_H

#include <cxxtest/TestSuite.h>
#include <cmath>

class DeltaTest : public CxxTest::TestSuite
{
    double _delta;
    
public:
    void setUp()
    {
        _delta = 0.0001;
    }

    void testSine()
    {
        TS_ASSERT_DELTA( sin(0.0), 0.0, _delta );
        TS_ASSERT_DELTA( sin(M_PI / 6), 0.5, _delta );
        TS_ASSERT_DELTA( sin(M_PI_2), 1.0, _delta );
        TS_ASSERT_DELTA( sin(M_PI), 0.0, _delta );
    }
};

#endif // __DELTATEST_H
