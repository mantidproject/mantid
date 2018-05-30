#ifndef MANTID_ALGORITHMS_INVERTMASKTEST_H_
#define MANTID_ALGORITHMS_INVERTMASKTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"

#include "MantidAlgorithms/InvertMask.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class InvertMaskTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InvertMaskTest *createSuite() { return new InvertMaskTest(); }
  static void destroySuite(InvertMaskTest *suite) { delete suite; }

  void test_NOTOperation() {

    InvertMask alg;
    alg.initialize();

    // 1. Create Mask Workspaces
    Mantid::Geometry::Instrument_sptr inst1 =
        ComponentCreationHelper::createTestInstrumentCylindrical(5);
    Mantid::DataObjects::MaskWorkspace_sptr ws1(
        new Mantid::DataObjects::MaskWorkspace(inst1));
    // ws1->setName("OriginalMask");
    AnalysisDataService::Instance().addOrReplace("OriginalMask", ws1);

    std::cout << "Input MaskWorkspace Size = " << ws1->getNumberHistograms()
              << '\n';

    ws1->setValue(1, 0);
    ws1->setValue(3, 1);

    // 2. Run
    std::string ws4name = "InvertedMask";

    alg.setProperty("InputWorkspace", ws1);
    alg.setPropertyValue("OutputWorkspace", ws4name);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    if (!alg.isExecuted())
      return;

    // 3. Get result
    DataObjects::MaskWorkspace_sptr ws4;
    ws4 =
        AnalysisDataService::Instance().retrieveWS<DataObjects::MaskWorkspace>(
            ws4name);
    TS_ASSERT(ws4);
    if (!ws4)
      return;

    TS_ASSERT_EQUALS(ws1->getNumberHistograms(), ws4->getNumberHistograms());
    if (ws1->getNumberHistograms() != ws4->getNumberHistograms())
      return;

    // 4. Check output
    for (size_t ih = 0; ih < ws4->getNumberHistograms(); ih++) {
      auto tempdetids = ws4->getDetectorIDs(ih);
      detid_t tempdetid = *(tempdetids.begin());
      TS_ASSERT_EQUALS(tempdetids.size(), 1);
      TS_ASSERT_DELTA(ws4->getValue(tempdetid), ws1->getValue(tempdetid), 1);
      TS_ASSERT_DELTA(ws4->y(ih)[0], ws1->y(ih)[0], 1);
    }

    return;
  }
};

#endif /* MANTID_ALGORITHMS_INVERTMASKTEST_H_ */
