// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidParallel/ThreadingBackend.h"

using Mantid::Parallel::detail::ThreadingBackend;

class ThreadingBackendTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ThreadingBackendTest *createSuite() { return new ThreadingBackendTest(); }
  static void destroySuite(ThreadingBackendTest *suite) { delete suite; }

  void test_default_constructor() {
    ThreadingBackend backend;
    TS_ASSERT_EQUALS(backend.size(), 1);
  }

  void test_size_constructor() {
    ThreadingBackend backend{2};
    TS_ASSERT_EQUALS(backend.size(), 2);
  }
};
