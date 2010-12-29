#ifndef ATOMTEST_H_
#define ATOMTEST_H_

#include <cxxtest/TestSuite.h>
#include <stdexcept>
#include "MantidKernel/Atom.h"
#include "MantidKernel/NeutronAtom.h"

using namespace Mantid::PhysicalConstants;

class AtomTest : public CxxTest::TestSuite
{
public:
  void testHydrogen()
  {
    Atom hydrogen = getAtom(1,2);
    Atom deuterium = getAtom("D");
    TS_ASSERT_EQUALS(hydrogen, deuterium);
    TS_ASSERT_EQUALS(hydrogen.z_number, 1);
    TS_ASSERT_EQUALS(hydrogen.a_number, 2);
    TS_ASSERT_EQUALS(hydrogen.abundance, 0.011500);
    TS_ASSERT_EQUALS(hydrogen.mass, 2.014102);
    //TS_ASSERT_EQUALS(hydrogen.neutron.coh_scatt_length_real, 6.671); // TODO
  }

  void testCm249()
  {
    Atom Cm249 = getAtom("Cm", 249);
    TS_ASSERT_EQUALS(Cm249.z_number, 96);
    TS_ASSERT_EQUALS(Cm249.a_number, 249);
    // cheap way to check for NaN
    //TS_ASSERT(Cm249.neutron.coh_scatt_length_real != Cm249.neutron.coh_scatt_length_real); // TODO
  }

  void testError()
  {
    TS_ASSERT_THROWS(getAtom(1,15), std::runtime_error);
    TS_ASSERT_THROWS(getAtom("garbage"), std::runtime_error);
  }
};

#endif // ATOMTEST_H_
