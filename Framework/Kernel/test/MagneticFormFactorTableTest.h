#ifndef MANTID_KERNEL_MAGNETICFORMFACTORTABLETEST_H_
#define MANTID_KERNEL_MAGNETICFORMFACTORTABLETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/MagneticFormFactorTable.h"

#include <boost/scoped_ptr.hpp>

#include <iomanip>

using Mantid::PhysicalConstants::MagneticFormFactorTable;

class MagneticFormFactorTableTest : public CxxTest::TestSuite {
public:
  void test_Table_Gives_Correct_Interplated_Value_For_Form_Factor() {
    using Mantid::PhysicalConstants::MagneticIon;
    const MagneticIon ion = Mantid::PhysicalConstants::getMagneticIon("Mn", 3);
    const uint16_t j(0), l(0);
    const size_t tableSize(500);
    const double qsqr(6.48);

    boost::scoped_ptr<MagneticFormFactorTable> lookup;
    TS_ASSERT_THROWS_NOTHING(
        lookup.reset(new MagneticFormFactorTable(tableSize, ion, j, l)));

    TS_ASSERT_DELTA(lookup->value(qsqr), 0.69202309, 1e-8);

    TS_ASSERT_DELTA(lookup->value(0.0), 0.9995, 1e-8); // On 0.0
    TS_ASSERT_DELTA(lookup->value(MagneticIon::formFactorCutOff(j, l)), 0.0,
                    1e-8); // On the cutoff
  }
};

#endif /* MANTID_KERNEL_MAGNETICFORMFACTORTABLETEST_H_ */
