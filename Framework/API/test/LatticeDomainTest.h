// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_LATTICEDOMAINTEST_H_
#define MANTID_API_LATTICEDOMAINTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/LatticeDomain.h"
#include "MantidKernel/Exception.h"

using Mantid::API::LatticeDomain;
using Mantid::Kernel::Exception::IndexError;
using Mantid::Kernel::V3D;

using namespace Mantid::API;

class LatticeDomainTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LatticeDomainTest *createSuite() { return new LatticeDomainTest(); }
  static void destroySuite(LatticeDomainTest *suite) { delete suite; }

  void testConstruction() {
    std::vector<V3D> hkls{{1, 1, 1}, {2, 1, 0}, {0, 0, 1}};

    TS_ASSERT_THROWS_NOTHING(LatticeDomain domain(hkls));

    std::vector<V3D> empty;
    TS_ASSERT_THROWS_NOTHING(LatticeDomain domain(empty));
  }

  void testSize() {
    std::vector<V3D> hkls{{1, 1, 1}, {2, 1, 0}, {0, 0, 1}};

    LatticeDomain domain(hkls);

    TS_ASSERT_EQUALS(domain.size(), hkls.size());
  }

  void testAccess() {
    std::vector<V3D> hkls{{1, 1, 1}, {2, 1, 0}, {0, 0, 1}};

    LatticeDomain domain(hkls);

    TS_ASSERT_THROWS_NOTHING(domain[0]);
    TS_ASSERT_THROWS_NOTHING(domain[1]);
    TS_ASSERT_THROWS_NOTHING(domain[2]);

    TS_ASSERT_THROWS(domain[3], const IndexError &)
  }
};

#endif /* MANTID_API_LATTICEDOMAINTEST_H_ */
