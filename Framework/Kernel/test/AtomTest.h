// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ATOMTEST_H_
#define ATOMTEST_H_

#include "MantidKernel/Atom.h"
#include "MantidKernel/NeutronAtom.h"
#include <cxxtest/TestSuite.h>
#include <stdexcept>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

using namespace Mantid::PhysicalConstants;

class AtomTest : public CxxTest::TestSuite {
public:
  void testHydrogen() {
    Atom hydrogen = getAtom(1, 2);
    Atom deuterium = getAtom("D");
    TS_ASSERT_EQUALS(hydrogen, deuterium);
    TS_ASSERT_EQUALS(hydrogen.z_number, 1);
    TS_ASSERT_EQUALS(hydrogen.a_number, 2);
    TS_ASSERT_EQUALS(hydrogen.abundance, 0.011500);
    TS_ASSERT_EQUALS(hydrogen.mass, 2.014102);
    // TS_ASSERT_EQUALS(hydrogen.neutron.coh_scatt_length_real, 6.671); // TODO
  }

  void testCm249() {
    Atom Cm249 = getAtom("Cm", 249);
    TS_ASSERT_EQUALS(Cm249.z_number, 96);
    TS_ASSERT_EQUALS(Cm249.a_number, 249);
    // cheap way to check for NaN
    // TS_ASSERT(Cm249.neutron.coh_scatt_length_real !=
    // Cm249.neutron.coh_scatt_length_real); // TODO
  }

  void test_Z_Number() {
    for (uint16_t z = 1; z <= 96; ++z) {
      Atom a = getAtom(z);
      TS_ASSERT_EQUALS(a.z_number, z);
    }
  }

  void testError() {
    TS_ASSERT_THROWS(getAtom(1, 15), const std::runtime_error &);
    TS_ASSERT_THROWS(getAtom("garbage"), const std::runtime_error &);
  }
};

class AtomTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AtomTestPerformance *createSuite() {
    return new AtomTestPerformance();
  }
  static void destroySuite(AtomTestPerformance *suite) { delete suite; }

  /// Set up all the test workspaces
  AtomTestPerformance() {
    const size_t test_size = 1000000;
    boost::random::mt19937 gen;
    boost::random::uniform_int_distribution<uint16_t> dist(1, 96);
    for (size_t i = 0; i < test_size; ++i) {
      z_input.push_back(dist(gen));
    }
    for (auto z : z_input) {
      symbol_input.push_back(getAtom(z).symbol);
    }
  }

#ifdef _MSC_VER

#pragma optimize("", off)

  static void escape(void *p) { p = p; }

#pragma optimize("", on)

#else

  static void escape(void *p) { asm volatile("" : : "g"(p) : "memory"); }

#endif

  void test_z_performance() {
    for (uint16_t z : z_input) {
      const Atom &a = getAtom(z);
      escape(const_cast<Atom *>(&a));
    }
  }

  void test_symbol_performance() {
    for (const auto &symbol : symbol_input) {
      const Atom &a = getAtom(symbol);
      escape(const_cast<Atom *>(&a));
    }
  }

private:
  std::vector<uint16_t> z_input;
  std::vector<std::string> symbol_input;
};

#endif // ATOMTEST_H_
