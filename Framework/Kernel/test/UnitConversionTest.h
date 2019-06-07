// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_UNITCONVERTERTEST_H_
#define MANTID_KERNEL_UNITCONVERTERTEST_H_

#include "MantidKernel/Exception.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitConversion.h"
#include <cxxtest/TestSuite.h>

using Mantid::Kernel::UnitConversion;

class UnitConversionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static UnitConversionTest *createSuite() { return new UnitConversionTest(); }
  static void destroySuite(UnitConversionTest *suite) { delete suite; }

  void test_Run_Throws_When_Src_Unit_Is_Unknown() {
    using namespace Mantid::Kernel::Exception;
    using Mantid::Kernel::DeltaEMode;
    TS_ASSERT_THROWS(UnitConversion::run("zxzxz", "Wavelength", 0.0, 0.0, 0.0,
                                         0.0, DeltaEMode::Elastic, 0.0),
                     const NotFoundError &);
  }

  void test_Run_Throws_When_Dest_Unit_Is_Unknown() {
    using namespace Mantid::Kernel::Exception;
    using Mantid::Kernel::DeltaEMode;
    TS_ASSERT_THROWS(UnitConversion::run("Wavelength", "xszfsdf", 0.0, 0.0, 0.0,
                                         0.0, DeltaEMode::Elastic, 0.0),
                     const NotFoundError &);
  }

  void
  test_Run_Gives_Correct_Value_For_Units_That_Can_Be_Converted_By_Simply_Factor_And_Geometry_Is_Ignored() {
    using Mantid::Kernel::DeltaEMode;

    const std::string &srcUnit = "Wavelength";
    const double srcValue(1.5); // In angstroms

    const std::string &destUnit = "Momentum";
    const double dummy(0.0);
    const DeltaEMode::Type dummyMode(DeltaEMode::Indirect);
    const double expected(2.0 * M_PI / srcValue);

    double result(-1.0);
    TS_ASSERT_THROWS_NOTHING(
        result = UnitConversion::run(srcUnit, destUnit, srcValue, dummy, dummy,
                                     dummy, dummyMode, dummy));
    TS_ASSERT_DELTA(result, expected, 1e-12);
  }

  void test_Run_Gives_Correct_Value_For_Units_That_Go_Through_TOF() {
    using Mantid::Kernel::DeltaEMode;

    const std::string &srcUnit = "Wavelength";
    const double srcValue(1.5); // In angstroms
    const std::string &destUnit = "MomentumTransfer";

    const double l1(10.0), l2(1.1), theta(10.0 * M_PI / 180.0), efixed(12.0);
    const DeltaEMode::Type emode = DeltaEMode::Direct;

    const double expected(0.437943919458);
    double result(-1.0);
    TS_ASSERT_THROWS_NOTHING(
        result = UnitConversion::run(srcUnit, destUnit, srcValue, l1, l2, theta,
                                     emode, efixed));
    TS_ASSERT_DELTA(result, expected, 1e-12);
  }
};

#endif /* MANTID_KERNEL_UNITCONVERTERTEST_H_ */
