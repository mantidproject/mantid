#ifndef NEUTRONATOMTEST_H_
#define NEUTRONATOMTEST_H_

#include <cxxtest/TestSuite.h>
#include <stdexcept>
#include "MantidKernel/NeutronAtom.h"

using namespace Mantid::PhysicalConstants;

class NeutronAtomTest : public CxxTest::TestSuite
{
public:
  void testHydrogen()
  {
    NeutronAtom hydrogen = getNeutronAtom(1);
    TS_ASSERT_EQUALS(hydrogen.z_number, 1);
    TS_ASSERT_EQUALS(hydrogen.a_number, 0);
    TS_ASSERT_EQUALS(hydrogen.abs_scatt_xs, 0.3326);
  }

  void testCurium()
  {
    NeutronAtom curium = getNeutronAtom(96, 248);
    TS_ASSERT_EQUALS(curium.z_number, 96);
    TS_ASSERT_EQUALS(curium.a_number, 248);
    TS_ASSERT_EQUALS(curium.coh_scatt_length_real, 7.7);
  }

  void testError()
  {
    TS_ASSERT_THROWS(getNeutronAtom(1, 15), std::runtime_error);
    TS_ASSERT_THROWS(getNeutronAtom(97), std::runtime_error);
  }
};

#endif // NEUTRONATOMTEST_H_
