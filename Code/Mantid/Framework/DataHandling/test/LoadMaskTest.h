#ifndef MANTID_DATAHANDLING_LOADMASKTEST_H_
#define MANTID_DATAHANDLING_LOADMASKTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include "Poco/File.h"

#include "MantidDataHandling/LoadMask.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class LoadMaskTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadMaskTest *createSuite() { return new LoadMaskTest(); }
  static void destroySuite( LoadMaskTest *suite ) { delete suite; }


  void test_LoadXML()
  {
    LoadMask loadfile;
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

  /*
   * By given a non-existing instrument's name, exception should be thrown.
   */
  void test_LoadXMLThrow()
  {
    LoadMask loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", "WhatEver");
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

  /*
   * Test mask by detector ID
   * For VULCAN:
   * workspaceindex:  detector ID
   * 34           :   26284
   * 1000         :   27250
   * 2000         :   28268
   */
  void test_DetectorIDs()
  {
    // 1. Generate masking files
    std::vector<int> banks1;
    std::vector<int> detids;
    detids.push_back(26284);
    detids.push_back(27250);
    detids.push_back(28268);
    std::string maskfname1("maskingdet.xml");
    genMaskingFile(maskfname1, detids, banks1);

    // 2. Run
    LoadMask loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", "VULCAN");
    loadfile.setProperty("InputFile", maskfname1);
    loadfile.setProperty("OutputWorkspace", "VULCAN_Mask_Detectors");

    TS_ASSERT_EQUALS(loadfile.execute(),true);
    DataObjects::SpecialWorkspace2D_sptr maskws =
          boost::dynamic_pointer_cast<DataObjects::SpecialWorkspace2D>(AnalysisDataService::Instance().retrieve("VULCAN_Mask_Detectors"));

    // 3. Check
    for (size_t iws=0; iws<maskws->getNumberHistograms(); iws++)
    {
      double y = maskws->dataY(iws)[0];
      if (iws==34 || iws==1000 || iws==2000)
      {
        // These 3 workspace index are masked
        TS_ASSERT_DELTA(y, 1.0, 1.0E-5);
      }
      else
      {
        // Unmasked
        TS_ASSERT_DELTA(y, 0.0, 1.0E-5);
      }
    }

    // 4. Clean
    Poco::File cleanfile(maskfname1);
    cleanfile.remove(false);

    return;
  }



  /*
   * Load "testingmasking.xml" and "regionofinterest.xml"
   * These two xml files will generate two opposite Workspaces, i.e.,
   * Number(masked detectors of WS1) = Number(unmasked detectors of WS2)
   *
   * by BinaryOperation
   */
  void test_Banks()
  {
    // 0. Generate masking files
    std::vector<int> banks1;
    banks1.push_back(21);
    banks1.push_back(22);
    std::vector<int> detids;
    std::string maskfname1("masking01.xml");
    genMaskingFile(maskfname1, detids, banks1);

    std::vector<int> banks2;
    banks2.push_back(23);
    banks2.push_back(26);
    banks2.push_back(27);
    banks2.push_back(28);
    std::string maskfname2("masking02.xml");
    genMaskingFile(maskfname2, detids, banks2);

    // 1. Generate Mask Workspace
    LoadMask loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", "VULCAN");
    loadfile.setProperty("InputFile", "masking01.xml");
    loadfile.setProperty("OutputWorkspace", "VULCAN_Mask1");

    TS_ASSERT_EQUALS(loadfile.execute(),true);
    DataObjects::SpecialWorkspace2D_sptr maskws =
          boost::dynamic_pointer_cast<DataObjects::SpecialWorkspace2D>(AnalysisDataService::Instance().retrieve("VULCAN_Mask1"));

    // 2. Generate Region of Interest Workspace
    LoadMask loadfile2;
    loadfile2.initialize();

    loadfile2.setProperty("Instrument", "VULCAN");
    loadfile2.setProperty("InputFile", "masking02.xml");
    loadfile2.setProperty("OutputWorkspace", "VULCAN_Mask2");

    TS_ASSERT_EQUALS(loadfile2.execute(), true);
    DataObjects::SpecialWorkspace2D_sptr interestws =
          boost::dynamic_pointer_cast<DataObjects::SpecialWorkspace2D>(AnalysisDataService::Instance().retrieve("VULCAN_Mask2"));

    // 3. Check
    size_t sizemask = maskws->getNumberHistograms();
    size_t sizeinterest = interestws->getNumberHistograms();
    TS_ASSERT_EQUALS(sizemask==sizeinterest, true);

    if (sizemask == sizeinterest){
      // number1: number of masked detectors of maskws
      // number2: number of used detectors of interestws
      size_t number1 = 0;
      size_t number0 = 0;
      for (size_t ih = 0; ih < maskws->getNumberHistograms(); ih ++){
        double v1 = maskws->dataY(ih)[0];
        double v2 = interestws->dataY(ih)[0];
        if (v1 > 0.5){
          number0 ++;
        }
        if (v2 < 0.5){
          number1 ++;
        }
        TS_ASSERT_EQUALS(v1+v2<1.5, true); // must be 1
        TS_ASSERT_EQUALS(v1+v2>0.5, true);
      }

      TS_ASSERT_EQUALS(number0 > 0, true);
      TS_ASSERT_EQUALS(number1 > 0, true);
      TS_ASSERT_EQUALS(number1-number0, 0);
    }

    // 4. Delete
    Poco::File cleanfile1(maskfname1);
    cleanfile1.remove(false);

    Poco::File cleanfile2(maskfname2);
    cleanfile2.remove(false);

    return;
  } // test_Openfile

  /*
   * Create a masking file
   */
  void genMaskingFile(std::string maskfilename, std::vector<int> detids, std::vector<int> banks)
  {

    std::ofstream ofs;
    ofs.open(maskfilename.c_str(), std::ios::out);

    // 1. Header
    ofs << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << std::endl;
    ofs << "  <detector-masking>" << std::endl;
    ofs << "    <group>" << std::endl;

    // 2. "detids" & component
    if (detids.size() > 0)
    {
      ofs << "    <detids>";
      for (size_t i=0; i < detids.size(); i++)
      {
        if (i < detids.size()-1)
          ofs << detids[i] << ",";
        else
          ofs << detids[i];
      }
      ofs << "</detids>" << std::endl;
    }

    for (size_t i=0; i < banks.size(); i++)
    {
      ofs << "<component>bank" << banks[i] << "</component>" << std::endl;
    }

    // 4. End of file
    ofs << "  </group>" << std::endl << "</detector-masking>" << std::endl;

    ofs.close();

    return;
  }
};


#endif /* MANTID_DATAHANDLING_LOADMASKINGFILETEST_H_ */

