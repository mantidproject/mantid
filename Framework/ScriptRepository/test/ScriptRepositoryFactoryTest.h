// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ScriptRepositoryFactory.h"
#include "MantidScriptRepository/ScriptRepositoryImpl.h"

using namespace Mantid;
using namespace Mantid::API;

class ScriptRepositoryFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ScriptRepositoryFactoryTest *createSuite() { return new ScriptRepositoryFactoryTest(); }
  static void destroySuite(ScriptRepositoryFactoryTest *suite) { delete suite; }

  ScriptRepositoryFactoryTest() { Mantid::API::FrameworkManager::Instance(); }

  void testCreateScriptRepository() {
    ScriptRepository_sptr script = ScriptRepositoryFactory::Instance().create("ScriptRepositoryImpl");
    TS_ASSERT(script);
  }
};
