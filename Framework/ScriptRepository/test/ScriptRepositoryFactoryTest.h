#ifndef FUNCTIONFACTORYTEST_H_
#define FUNCTIONFACTORYTEST_H_

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
  static ScriptRepositoryFactoryTest *createSuite() {
    return new ScriptRepositoryFactoryTest();
  }
  static void destroySuite(ScriptRepositoryFactoryTest *suite) { delete suite; }

  ScriptRepositoryFactoryTest() { Mantid::API::FrameworkManager::Instance(); }

  void testCreateScriptRepository() {
    ScriptRepository_sptr script =
        ScriptRepositoryFactory::Instance().create("ScriptRepositoryImpl");
    TS_ASSERT(script);
  }
};

#endif /*SCRIPTREPOSITORYFACTORYTEST_H_*/
