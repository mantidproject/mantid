// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef UNITFACTORYTEST_H_
#define UNITFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include <boost/shared_ptr.hpp>

using namespace Mantid::Kernel;

class UnitFactoryTest : public CxxTest::TestSuite {
public:
  void test_Create_With_Valid_Unit_Gives_Valid_Pointer() {
    boost::shared_ptr<Unit> first;
    TS_ASSERT_THROWS_NOTHING(first = UnitFactory::Instance().create("TOF"));
    TSM_ASSERT(
        "UnitFactory::create did not throw but it returned an empty pointer",
        first);
  }

  void test_Create_With_Unknown_Unit_Throws_Exception() {
    TS_ASSERT_THROWS(UnitFactory::Instance().create("_NOT_A_REAL_UNIT"),
                     const Exception::NotFoundError &);
  }
};

#endif /*UNITFACTORYTEST_H_*/
