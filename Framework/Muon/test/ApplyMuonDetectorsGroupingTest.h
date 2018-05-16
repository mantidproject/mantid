#ifndef MANTID_MUON_APPLYMUONDETECTORSGROUPINGTEST_H_
#define MANTID_MUON_APPLYMUONDETECTORSGROUPINGTEST_H_


#include <cxxtest/TestSuite.h>

#include "MantidAPI/TableRow.h"
#include "MantidAlgorithms/MuonGroupDetectors.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::MuonGroupDetectors;

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;


class ApplyMuonDetectorsGroupingTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ApplyMuonDetectorsGroupingTest *createSuite() {
    return new ApplyMuonDetectorsGroupingTest();
  }
  static void destroySuite(ApplyMuonDetectorsGroupingTest *suite) { delete suite; }


void test_Init() {
    MuonGroupDetectors alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

}

#endif /* MANTID_MUON_APPLYMUONDETECTORSGROUPINGTEST_H_ */