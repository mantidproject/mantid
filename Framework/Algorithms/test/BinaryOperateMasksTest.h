#ifndef MANTID_ALGORITHMS_BINARYOPERATEMASKSTEST_H_
#define MANTID_ALGORITHMS_BINARYOPERATEMASKSTEST_H_

#include "MantidAlgorithms/BinaryOperateMasks.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class BinaryOperateMasksTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BinaryOperateMasksTest *createSuite() {
    return new BinaryOperateMasksTest();
  }
  static void destroySuite(BinaryOperateMasksTest *suite) { delete suite; }

  void test_TwoInputWorkspaces() {

    this->binoperator.initialize();

    // 1. Create Mask Workspace
    Mantid::Geometry::Instrument_sptr inst1 =
        ComponentCreationHelper::createTestInstrumentCylindrical(5);
    // Mantid::Geometry::Instrument_sptr inst1(new Geometry::Instrument);
    Mantid::DataObjects::MaskWorkspace_sptr ws1(
        new Mantid::DataObjects::MaskWorkspace(inst1));
    Mantid::DataObjects::MaskWorkspace_const_sptr cws1 =
        boost::dynamic_pointer_cast<const Mantid::DataObjects::MaskWorkspace>(
            ws1);

    Mantid::Geometry::Instrument_sptr inst2 =
        ComponentCreationHelper::createTestInstrumentCylindrical(5);
    Mantid::DataObjects::MaskWorkspace_sptr ws2(
        new Mantid::DataObjects::MaskWorkspace(inst2));

    std::string ws3name = "BinarySum";

    ws1->setValue(1, 0);
    ws2->setValue(1, 1);

    ws1->setValue(2, 0);
    ws2->setValue(2, 0);

    this->binoperator.setProperty("InputWorkspace1", ws1);
    this->binoperator.setProperty("InputWorkspace2", ws2);
    this->binoperator.setPropertyValue("OperationType", "OR");

    this->binoperator.setPropertyValue("OutputWorkspace", ws3name);

    try {
      TS_ASSERT_EQUALS(this->binoperator.execute(), true);

      DataObjects::MaskWorkspace_sptr ws3 =
          AnalysisDataService::Instance()
              .retrieveWS<DataObjects::MaskWorkspace>(ws3name);

      TS_ASSERT_EQUALS(ws3->getValue(1), 1);
      TS_ASSERT_EQUALS(ws3->getValue(2), 0);

    } catch (std::runtime_error &e) {
      TS_FAIL(e.what());
    }

    std::cout << "\nTest I Is Completed\n";

    if (ws1 == nullptr) {
      std::cout << "\nWorkspace1 is NULL\n";
    }

  } // End test_TwoInputWorkspaces

  void test_NOTOperation() {
    this->binoperator.initialize();

    // 1. Create Mask Workspaces
    Mantid::Geometry::Instrument_sptr inst1 =
        ComponentCreationHelper::createTestInstrumentCylindrical(5);
    Mantid::DataObjects::MaskWorkspace_sptr ws1(
        new Mantid::DataObjects::MaskWorkspace(inst1));

    ws1->setValue(1, 0);
    ws1->setValue(3, 1);

    this->binoperator.setProperty("InputWorkspace1", ws1);
    std::string ws4name = "BinaryNOTResult";
    this->binoperator.setPropertyValue("OutputWorkspace", ws4name);
    this->binoperator.setPropertyValue("OperationType", "NOT");
    DataObjects::MaskWorkspace_sptr ws4;
    try {
      TS_ASSERT_EQUALS(this->binoperator.execute(), true);
      ws4 = AnalysisDataService::Instance()
                .retrieveWS<DataObjects::MaskWorkspace>(ws4name);

      if (ws4 == nullptr) {
        std::cout << "Workspace4 is NULL\n";
      } else {
        std::cout << "Workspace4 is good at output of NOT.  Number Histogram = "
                  << ws4->getNumberHistograms() << '\n';
      }
      if (ws1 == nullptr) {
        std::cout << "Workspace1 is NULL\n";
      } else {
        std::cout << "Workspace1 is good at output of NOT.  Number Histogram = "
                  << ws1->getNumberHistograms() << '\n';
      }

      for (size_t ih = 0; ih < ws4->getNumberHistograms(); ih++) {
        detid_t tempdetid = *(ws4->getDetectorIDs(ih).begin());
        TS_ASSERT_DELTA(ws4->getValue(tempdetid), ws1->getValue(tempdetid), 1);
        // std::cout << ih << " - " << tempdetid << ": " <<
        // ws1->getValue(tempdetid) << " vs. " << ws4->getValue(tempdetid) <<
        // '\n';
      }
    } catch (std::runtime_error &e) {
      TS_FAIL(e.what());
    }

    this->binoperator.setProperty("InputWorkspace1", ws1);
    this->binoperator.setProperty("InputWorkspace2", ws4);
    std::string ws2name = "BinaryXorResult";
    this->binoperator.setPropertyValue("OutputWorkspace", ws2name);
    this->binoperator.setPropertyValue("OperationType", "XOR");
    try {
      TS_ASSERT_EQUALS(this->binoperator.execute(), true);
      DataObjects::MaskWorkspace_sptr ws2 =
          AnalysisDataService::Instance()
              .retrieveWS<DataObjects::MaskWorkspace>(ws2name);
      for (size_t ih = 0; ih < ws2->getNumberHistograms(); ih++) {
        detid_t tempdetid = *(ws2->getDetectorIDs(ih).begin());
        TS_ASSERT_EQUALS(ws2->getValue(tempdetid), 1);
      }
    } catch (std::runtime_error &e) {
      TS_FAIL(e.what());
    }
  }

private:
  // Define the algorithm
  BinaryOperateMasks binoperator;
};

#endif /* MANTID_ALGORITHMS_BINARYOPERATEMASKSTEST_H_ */
