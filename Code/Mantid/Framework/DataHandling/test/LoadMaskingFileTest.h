#ifndef MANTID_DATAHANDLING_LOADMASKINGFILETEST_H_
#define MANTID_DATAHANDLING_LOADMASKINGFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/LoadMaskingFile.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class LoadMaskingFileTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadMaskingFileTest *createSuite() { return new LoadMaskingFileTest(); }
  static void destroySuite( LoadMaskingFileTest *suite ) { delete suite; }


  void test_LoadXML()
  {
    LoadMaskingFile loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", "POWGEN");
    loadfile.setProperty("InputFile", "testmasking.xml");
    loadfile.setProperty("OutputWorkspace", "PG3Mask");

    try
    {
      TS_ASSERT_EQUALS(loadfile.execute(), true);
      DataObjects::SpecialWorkspace2D_sptr maskws =
          boost::dynamic_pointer_cast<DataObjects::SpecialWorkspace2D>(AnalysisDataService::Instance().retrieve("PG3Mask"));
    }
    catch(std::runtime_error & e)
    {
      TS_FAIL(e.what());
    }
  } // test_LoadXML


  void test_LoadXML2()
  {
    LoadMaskingFile loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", "NOMAD");
    loadfile.setProperty("InstrumentName", "POWGEN");
    loadfile.setProperty("InputFile", "testmasking.xml");
    loadfile.setProperty("OutputWorkspace", "PG3Mask");

    try
    {
      TS_ASSERT_EQUALS(loadfile.execute(), true);
      DataObjects::SpecialWorkspace2D_sptr maskws =
          boost::dynamic_pointer_cast<DataObjects::SpecialWorkspace2D>(AnalysisDataService::Instance().retrieve("PG3Mask"));
      std::string instrumentname = maskws->getInstrument()->getName();
      int r = instrumentname.compare("POWGEN");
      TS_ASSERT_EQUALS(r, 0);
    }
    catch(std::runtime_error & e)
    {
      TS_FAIL(e.what());
    }

  } // test_LoadXML



  void test_LoadXMLThrow()
  {
    LoadMaskingFile loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", "NOMAD");
    loadfile.setProperty("InstrumentName", "WhatEver");
    loadfile.setProperty("InputFile", "testmasking.xml");
    loadfile.setProperty("OutputWorkspace", "PG3Mask");

    try
    {
      TS_ASSERT_EQUALS(loadfile.execute(), false);
    }
    catch(std::runtime_error & e)
    {
      TS_FAIL(e.what());
    }

  } // test_LoadXML

  void test_BinaryOperation()
  {
    // 1. Generate Mask Workspace
    LoadMaskingFile loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", "POWGEN");
    loadfile.setProperty("InputFile", "testmasking.xml");
    loadfile.setProperty("OutputWorkspace", "PG3Mask");

    TS_ASSERT_EQUALS(loadfile.execute(),true);
    DataObjects::SpecialWorkspace2D_sptr maskws =
          boost::dynamic_pointer_cast<DataObjects::SpecialWorkspace2D>(AnalysisDataService::Instance().retrieve("PG3Mask"));

    // 2. Generate Region of Interest Workspace
    LoadMaskingFile loadfile2;
    loadfile2.initialize();

    loadfile2.setProperty("Instrument", "POWGEN");
    loadfile2.setProperty("InputFile", "regionofinterest.xml");
    loadfile2.setProperty("OutputWorkspace", "PG3Interest");

    TS_ASSERT_EQUALS(loadfile2.execute(), true);
    DataObjects::SpecialWorkspace2D_sptr interestws =
          boost::dynamic_pointer_cast<DataObjects::SpecialWorkspace2D>(AnalysisDataService::Instance().retrieve("PG3Interest"));

    // 3. Check
    size_t sizemask = maskws->getNumberHistograms();
    size_t sizeinterest = interestws->getNumberHistograms();
    TS_ASSERT_EQUALS(sizemask==sizeinterest, true);

    if (sizemask == sizeinterest){
      size_t number1 = 0;
      size_t number0 = 0;
      for (size_t ih = 0; ih < maskws->getNumberHistograms(); ih ++){
        double v1 = maskws->dataY(ih)[0];
        double v2 = interestws->dataY(ih)[0];
        if (v1 < 0.5){
          number0 ++;
        }
        if (v2 > 0.5){
          number1 ++;
        }
        TS_ASSERT_EQUALS(v1+v2<1.5, true);
        TS_ASSERT_EQUALS(v1+v2>0.5, true);
      }

      TS_ASSERT_EQUALS(number0 > 0, true);
      TS_ASSERT_EQUALS(number1 > 0, true);
      TS_ASSERT_EQUALS(number1-number0, 0);
    }

  } // test_Openfile

};


#endif /* MANTID_DATAHANDLING_LOADMASKINGFILETEST_H_ */

