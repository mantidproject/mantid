// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadDAE.h"

using namespace Mantid::DataHandling;

class LoadDAETest : public CxxTest::TestSuite {
public:
  static LoadDAETest *createSuite() { return new LoadDAETest(); }
  static void destroySuite(LoadDAETest *suite) { delete suite; }

  void testInit() {
    LoadDAE loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
  }

  void testDeprecated() {
    LoadDAE loader;
    TS_ASSERT(dynamic_cast<Mantid::API::DeprecatedAlgorithm *>(&loader));
    TS_ASSERT_EQUALS(loader.deprecationMsg(&loader),
                     "LoadDAE is deprecated (on 2013-04-22). Use StartLiveData instead.");
  }
};
