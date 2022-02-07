// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include <stdexcept>

using namespace Mantid::Kernel;
using namespace Mantid::API;

class FrameworkManagerTest : public CxxTest::TestSuite {
public:
  // This means the constructor isn't called when running other tests
  static FrameworkManagerTest *createSuite() { return new FrameworkManagerTest(); }
  static void destroySuite(FrameworkManagerTest *suite) { delete suite; }

#ifdef MPI_EXPERIMENTAL
  // Make sure FrameworkManager is always instantiated. This is needed to
  // initialize the MPI environment.
  FrameworkManagerTest() { FrameworkManager::Instance(); }
#endif

  void testConstructor() {
    // Not really much to test
    TS_ASSERT_THROWS_NOTHING(FrameworkManager::Instance());

#ifdef MPI_BUILD
    // If this is 'MPI Mantid' then test that the mpi environment has been
    // initialized
    TS_ASSERT(boost::mpi::environment::initialized());
#endif
  }

  void testGetWorkspace() {
    TS_ASSERT_THROWS(FrameworkManager::Instance().getWorkspace("wrongname"), const std::runtime_error &)
  }
};
