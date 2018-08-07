#ifndef MANTID_DATAHANDLING_SAVEISAWDETCALTEST_H_
#define MANTID_DATAHANDLING_SAVEISAWDETCALTEST_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/SaveIsawDetCal.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <Poco/File.h>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class SaveIsawDetCalTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveIsawDetCalTest *createSuite() { return new SaveIsawDetCalTest(); }
  static void destroySuite(SaveIsawDetCalTest *suite) { delete suite; }

  void test_Init() {
    SaveIsawDetCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    EventWorkspace_sptr ws =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(2, 50);

    SaveIsawDetCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("Filename", "SaveIsawDetCalTest.DetCal"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "InputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(ws)));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename).exists());
    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }
};

#endif /* MANTID_DATAHANDLING_SAVEISAWDETCALTEST_H_ */
