// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef LOADNGEMTEST_H_
#define LOADNGEMTEST_H_

#include <cxxtest/TestSuite.h>

class LoadNGEMTest : public CxxTest::TestSuite {
public:
  void testPlaceholder() { TS_ASSERT_EQUALS(1, 1); }
};

#endif // LOADNGEMTEST_H_