#ifndef MANTID_ALGORITHMS_BINARYOPERATEMASKSTEST_H_
#define MANTID_ALGORITHMS_BINARYOPERATEMASKSTEST_H_

#include "MantidAlgorithms/BinaryOperateMasks.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include <cxxtest/TestSuite.h>
#include <iostream>
#include <iomanip>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class BinaryOperateMasksTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BinaryOperateMasksTest *createSuite() { return new BinaryOperateMasksTest(); }
  static void destroySuite( BinaryOperateMasksTest *suite ) { delete suite; }


  void test_TwoInputWorkspaces()
  {

    this->binoperator.initialize();

    // 1. Create SpecialWorkspaces
    Mantid::Geometry::Instrument_sptr inst1 = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    // Mantid::Geometry::Instrument_sptr inst1(new Geometry::Instrument);
    Mantid::DataObjects::SpecialWorkspace2D_sptr ws1(new  Mantid::DataObjects::SpecialWorkspace2D(inst1));
    Mantid::DataObjects::SpecialWorkspace2D_const_sptr cws1 = boost::dynamic_pointer_cast<const Mantid::DataObjects::SpecialWorkspace2D>(ws1);

    Mantid::Geometry::Instrument_sptr inst2 = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    Mantid::DataObjects::SpecialWorkspace2D_sptr ws2(new Mantid::DataObjects::SpecialWorkspace2D(inst2));

    std::string ws3name = "BinarySum";

    ws1->setValue(1, 0);
    ws2->setValue(1, 1);

    ws1->setValue(2, 0);
    ws2->setValue(2, 0);

    this->binoperator.setProperty("InputWorkspace1", ws1);
    this->binoperator.setProperty("InputWorkspace2", ws2);
    this->binoperator.setPropertyValue("OperationType", "OR");

    this->binoperator.setPropertyValue("OutputWorkspace", ws3name);

    try
    {
      TS_ASSERT_EQUALS(this->binoperator.execute(),true);

      // DataObjects::SpecialWorkspace2D_sptr ws3 = this->binoperator.getProperty("OutputWorkspace");
      DataObjects::SpecialWorkspace2D_sptr ws3 = AnalysisDataService::Instance().retrieveWS<DataObjects::SpecialWorkspace2D>(ws3name);

      TS_ASSERT_EQUALS(ws3->getValue(1), 1);
      TS_ASSERT_EQUALS(ws3->getValue(2), 0);

    }
    catch(std::runtime_error & e)
    {
      TS_FAIL(e.what());
    }

    std::cout << "\nTest I Is Completed" << std::endl;

    if (ws1 == NULL){
      std::cout << "\nWorkspace1 is NULL" << std::endl;
    }

  } // End test_TwoInputWorkspaces

  void test_NOTOperation(){
    this->binoperator.initialize();

    // 1. Create SpecialWorkspaces
    Mantid::Geometry::Instrument_sptr inst1 = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    Mantid::DataObjects::SpecialWorkspace2D_sptr ws1(new  Mantid::DataObjects::SpecialWorkspace2D(inst1));

    ws1->setValue(1, 0);
    ws1->setValue(3, 1);

    this->binoperator.setProperty("InputWorkspace1", ws1);
    std::string ws4name = "BinaryNOTResult";
    this->binoperator.setPropertyValue("OutputWorkspace", ws4name);
    this->binoperator.setPropertyValue("OperationType", "NOT");
    DataObjects::SpecialWorkspace2D_sptr ws4;
    try
    {
      TS_ASSERT_EQUALS(this->binoperator.execute(),true);
      ws4 = AnalysisDataService::Instance().retrieveWS<DataObjects::SpecialWorkspace2D>(ws4name);

      if (ws4 == NULL){
        std::cout << "Workspace4 is NULL" << std::endl;
      } else {
        std::cout << "Workspace4 is good at output of NOT.  Number Histogram = " << ws4->getNumberHistograms() << std::endl;
      }
      if (ws1 == NULL){
        std::cout << "Workspace1 is NULL" << std::endl;
      } else {
        std::cout << "Workspace1 is good at output of NOT.  Number Histogram = " << ws1->getNumberHistograms() << std::endl;
      }

      for (size_t ih = 0; ih < ws4->getNumberHistograms(); ih ++){
        detid_t tempdetid = ws4->getDetectorID(ih);
        TS_ASSERT_DELTA(ws4->getValue(tempdetid), ws1->getValue(tempdetid), 1);
        // std::cout << ih << " - " << tempdetid << ": " << ws1->getValue(tempdetid) << " vs. " << ws4->getValue(tempdetid) << std::endl;
      }
    }
    catch(std::runtime_error & e)
    {
      TS_FAIL(e.what());
    }

    this->binoperator.setProperty("InputWorkspace1", ws1);
    this->binoperator.setProperty("InputWorkspace2", ws4);
    std::string ws2name = "BinaryXorResult";
    this->binoperator.setPropertyValue("OutputWorkspace", ws2name);
    this->binoperator.setPropertyValue("OperationType", "XOR");
    try
    {
      TS_ASSERT_EQUALS(this->binoperator.execute(),true);
      DataObjects::SpecialWorkspace2D_sptr ws2 = AnalysisDataService::Instance().retrieveWS<DataObjects::SpecialWorkspace2D>(ws2name);
      for (size_t ih = 0; ih < ws2->getNumberHistograms(); ih ++){
        detid_t tempdetid = ws2->getDetectorID(ih);
        TS_ASSERT_EQUALS(ws2->getValue(tempdetid), 1);
      }
    }
    catch(std::runtime_error & e)
    {
      TS_FAIL(e.what());
    }

  }


private:
  // Define the algorithm
  BinaryOperateMasks binoperator;

};


#endif /* MANTID_ALGORITHMS_BINARYOPERATEMASKSTEST_H_ */

