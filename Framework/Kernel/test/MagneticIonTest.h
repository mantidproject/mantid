// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MAGNETICIONTEST_H_
#define MAGNETICIONTEST_H_

#include "MantidKernel/Exception.h"
#include "MantidKernel/MagneticIon.h"

#include <cxxtest/TestSuite.h>

#include <boost/lexical_cast.hpp>

using namespace Mantid::PhysicalConstants;

class MagneticIonTest : public CxxTest::TestSuite {
public:
  void testGetMagneticIonWithSeparateSymbolAndCharge() {
    MagneticIon temp = getMagneticIon("Am", 7);
    TS_ASSERT_EQUALS(temp.symbol, "Am");
    TS_ASSERT_EQUALS(temp.charge, 7);
    TS_ASSERT_DELTA(temp.j0[1], 12.73, 0.001);
  }

  void testGetMagneticIonWithCombinedSymbolAndCharge() {
    MagneticIon temp = getMagneticIon("Am7");
    TS_ASSERT_EQUALS(temp.symbol, "Am");
    TS_ASSERT_EQUALS(temp.charge, 7);
    TS_ASSERT_DELTA(temp.j0[1], 12.73, 0.001);
  }

  void testGetJL() {
    std::vector<double> temp;
    temp = getJL("Am", 7);
    TS_ASSERT_EQUALS(temp.size(), 8);
    TS_ASSERT_DELTA(temp[1], 12.73, 0.001);
  }

  void testErrors() {
    TS_ASSERT_THROWS(getMagneticIon("O", 2),
                     const std::runtime_error &); // no such ion
    TS_ASSERT_THROWS(getMagneticIon("Am", 12),
                     const std::runtime_error &); // no such charge
    TS_ASSERT_THROWS(
        getJL("Am", 12),
        const std::runtime_error &); // no such charge - pass to getJL
    TS_ASSERT_THROWS(getJL("Am", 7, 3),
                     const std::runtime_error &); // no such l
  }

  void test_Copied_Object_Has_Same_Attributes() {
    MagneticIon ion = getMagneticIon("Am", 7);
    MagneticIon copiedIon(ion);

    TS_ASSERT_EQUALS(ion.symbol, copiedIon.symbol);
    TS_ASSERT_EQUALS(ion.charge, copiedIon.charge);
    checkVectorsEqual("Checking j0 values:", ion.j0, copiedIon.j0);
    checkVectorsEqual("Checking j2 values:", ion.j2, copiedIon.j2);
    checkVectorsEqual("Checking j4 values:", ion.j4, copiedIon.j4);
    checkVectorsEqual("Checking j6 values:", ion.j6, copiedIon.j6);
  }

  void test_formFactor_Has_Expected_Value_For_Simplest_J0_L0_Case() {
    MagneticIon ion = getMagneticIon("Mn", 3);
    double qsqr = 6.48;
    TS_ASSERT_DELTA(ion.analyticalFormFactor(qsqr), 0.691958098, 1e-8);
  }

private:
  void checkVectorsEqual(const std::string &msgPrefix,
                         const std::vector<double> &first,
                         const std::vector<double> &second) {
    TSM_ASSERT_EQUALS(msgPrefix + " Size match", first.size(), second.size());
    if (first.size() != second.size())
      return;

    for (size_t i = 0; i < first.size(); ++i) {
      const std::string msg = msgPrefix + " Value mismatch at element " +
                              boost::lexical_cast<std::string>(i);
      TSM_ASSERT_DELTA(msg, first[i], second[i], 1e-12);
    }
  }
};

#endif // MAGNETICIONTEST_H_
