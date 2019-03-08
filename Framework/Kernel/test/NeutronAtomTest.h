// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef NEUTRONATOMTEST_H_
#define NEUTRONATOMTEST_H_

#include "MantidKernel/NeutronAtom.h"
#include <cxxtest/TestSuite.h>
#include <stdexcept>

using namespace Mantid::PhysicalConstants;

class NeutronAtomTest : public CxxTest::TestSuite {
public:
  void testHydrogen() {
    NeutronAtom hydrogen = getNeutronAtom(1);
    TS_ASSERT_EQUALS(hydrogen.z_number, 1);
    TS_ASSERT_EQUALS(hydrogen.a_number, 0);
    TS_ASSERT_EQUALS(hydrogen.abs_scatt_xs, 0.3326);
  }

  void testCurium() {
    NeutronAtom curium = getNeutronAtom(96, 248);
    TS_ASSERT_EQUALS(curium.z_number, 96);
    TS_ASSERT_EQUALS(curium.a_number, 248);
    TS_ASSERT_EQUALS(curium.coh_scatt_length_real, 7.7);
  }

  void testVanadium() {
    NeutronAtom atom = getNeutronAtom(23);
    TS_ASSERT_EQUALS(atom.z_number, 23);
    TS_ASSERT_EQUALS(atom.a_number, 0);
    TS_ASSERT_EQUALS(atom.coh_scatt_length_real, -0.3824);
    TS_ASSERT_EQUALS(atom.coh_scatt_length_img, 0.);
    TS_ASSERT_EQUALS(atom.inc_scatt_length_real, 0);
    TS_ASSERT_EQUALS(atom.inc_scatt_length_img, 0.);
    TS_ASSERT_EQUALS(atom.coh_scatt_xs, 0.0184);
    TS_ASSERT_EQUALS(atom.inc_scatt_xs, 5.08);
    TS_ASSERT_EQUALS(atom.tot_scatt_xs, 5.1);
    TS_ASSERT_EQUALS(atom.abs_scatt_xs, 5.08);
  }

  void testGadolinium() {
    NeutronAtom atom = getNeutronAtom(64);
    TS_ASSERT_EQUALS(atom.z_number, 64);
    TS_ASSERT_EQUALS(atom.a_number, 0);
    TS_ASSERT_EQUALS(atom.coh_scatt_length_real, 6.5);
    TS_ASSERT_EQUALS(atom.coh_scatt_length_img, -13.82);
    TS_ASSERT_EQUALS(atom.inc_scatt_length_real, 0);
    TS_ASSERT_EQUALS(atom.inc_scatt_length_img, 0);
    TS_ASSERT_EQUALS(atom.coh_scatt_xs, 29.3);
    TS_ASSERT_EQUALS(atom.inc_scatt_xs, 151.);
    TS_ASSERT_EQUALS(atom.tot_scatt_xs, 180.);
    TS_ASSERT_EQUALS(atom.abs_scatt_xs, 49700.);
  }

  void testError() {
    TS_ASSERT_THROWS(getNeutronAtom(1, 15), std::runtime_error);
    TS_ASSERT_THROWS(getNeutronAtom(97), std::runtime_error);
  }
};

#endif // NEUTRONATOMTEST_H_
