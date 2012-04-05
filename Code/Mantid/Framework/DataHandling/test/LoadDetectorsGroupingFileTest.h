#ifndef MANTID_DATAHANDLING_LOADDETECTORSGROUPINGFILETEST_H_
#define MANTID_DATAHANDLING_LOADDETECTORSGROUPINGFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include <fstream>

#include "MantidDataHandling/LoadDetectorsGroupingFile.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "Poco/File.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class LoadDetectorsGroupingFileTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadDetectorsGroupingFileTest *createSuite() { return new LoadDetectorsGroupingFileTest(); }
  static void destroySuite( LoadDetectorsGroupingFileTest *suite ) { delete suite; }


  void test_Init(){

    LoadDetectorsGroupingFile load;
    load.initialize();
    TS_ASSERT(load.isInitialized());

  }

  void test_DetectorsGroupingXMLFile()
  {
    /*
     * 0-3695 : 1.0
     * 3696-(-1): 2.0
     */

    LoadDetectorsGroupingFile load;
    load.initialize();

    TS_ASSERT(load.setProperty("InputFile", "vulcangroup.xml"));
    TS_ASSERT(load.setProperty("OutputWorkspace", "Vulcan_Group"));

    load.execute();
    TS_ASSERT(load.isExecuted());

    DataObjects::GroupingWorkspace_sptr gws = boost::dynamic_pointer_cast<DataObjects::GroupingWorkspace>(API::AnalysisDataService::Instance().retrieve("Vulcan_Group"));

    TS_ASSERT_DELTA(gws->dataY(0)[0],    1.0, 1.0E-5);
    TS_ASSERT_DELTA(gws->dataY(3695)[0], 1.0, 1.0E-5);
    TS_ASSERT_DELTA(gws->dataY(3696)[0], 2.0, 1.0E-5);
    TS_ASSERT_DELTA(gws->dataY(7000)[0], 2.0, 1.0E-5);

  }

  /*
   * Test on the group XML with zero
   */
  void test_AutoGroupIndex()
  {
    LoadDetectorsGroupingFile load;
    load.initialize();

    std::string xmlfilename = "testautoidgroup.xml";
    generateAutoGroupIDGroupXMLFile(xmlfilename);

    TS_ASSERT(load.setProperty("InputFile", xmlfilename));
    TS_ASSERT(load.setProperty("OutputWorkspace", "Vulcan_Group2"));

    load.execute();
    TS_ASSERT(load.isExecuted());

    DataObjects::GroupingWorkspace_sptr gws = boost::dynamic_pointer_cast<DataObjects::GroupingWorkspace>(API::AnalysisDataService::Instance().retrieve("Vulcan_Group"));

    TS_ASSERT_DELTA(gws->dataY(0)[0],    1.0, 1.0E-5);
    TS_ASSERT_DELTA(gws->dataY(3695)[0], 1.0, 1.0E-5);
    TS_ASSERT_DELTA(gws->dataY(3696)[0], 2.0, 1.0E-5);
    TS_ASSERT_DELTA(gws->dataY(7000)[0], 2.0, 1.0E-5);

    // Clean
    Poco::File cleanfile(xmlfilename);
    cleanfile.remove(false);

    return;
  }

  void generateAutoGroupIDGroupXMLFile(std::string xmlfilename)
  {
    std::ofstream ofs;
    ofs.open(xmlfilename.c_str(), std::ios::out);

    ofs << "<?xml version=\"1.0\"?>" << std::endl;
    ofs << "<detector-grouping instrument=\"VULCAN\">" << std::endl;
    ofs << "  <group>" << std::endl;
    ofs << "    <detids>26250-27481,27500-28731,28750-29981</detids>" << std::endl;
    ofs << "  </group>" << std::endl;
    ofs << "  <group>" << std::endl;
    ofs << "    <component>bank26</component>" << std::endl;
    ofs << "    <component>bank27</component>" << std::endl;
    ofs << "    <component>bank28</component>" << std::endl;
    ofs << "  </group>" << std::endl;
    ofs << "</detector-grouping>" << std::endl;

    ofs.close();

    return;
  }

  /*
   * Test XML file using "ids"
   */
  void test_SpectrumIDs()
  {
    LoadDetectorsGroupingFile load;
    load.initialize();

    std::string xmlfilename = "testnoinstrumentgroup.xml";
    generateSpectrumIDXMLFile(xmlfilename);

    TS_ASSERT(load.setProperty("InputFile", xmlfilename));
    TS_ASSERT(load.setProperty("OutputWorkspace", "Vulcan_Group3"));

    load.execute();
    TS_ASSERT(load.isExecuted());

    DataObjects::GroupingWorkspace_sptr gws = boost::dynamic_pointer_cast<DataObjects::GroupingWorkspace>(API::AnalysisDataService::Instance().retrieve("Vulcan_Group3"));

    TS_ASSERT_DELTA(gws->dataY(0)[0], 1.0, 1.0E-5);
    TS_ASSERT_DELTA(gws->dataY(1)[0], 1.0, 1.0E-5);
    TS_ASSERT_DELTA(gws->dataY(5)[0], 2.0, 1.0E-5);
    TS_ASSERT_DELTA(gws->dataY(16)[0], 2.0, 1.0E-5);

    // Clean
    Poco::File cleanfile(xmlfilename);
    cleanfile.remove(false);

    return;
  }

  void generateSpectrumIDXMLFile(std::string xmlfilename)
  {
    std::ofstream ofs;
    ofs.open(xmlfilename.c_str(), std::ios::out);

    ofs << "<?xml version=\"1.0\"?>" << std::endl;
    ofs << "<detector-grouping>" << std::endl;
    ofs << "  <group>" << std::endl;
    ofs << "    <ids>30-36,12-16,100-111</ids>" << std::endl;
    ofs << "  </group>" << std::endl;
    ofs << "  <group>" << std::endl;
    ofs << "    <ids>38</ids>" << std::endl;
    ofs << "    <ids>291</ids>" << std::endl;
    ofs << "    <ids>22-25</ids>" << std::endl;
    ofs << "  </group>" << std::endl;
    ofs << "  <group name=\"bwd2\"><ids val=\"333,444,555\"/>334,557</group>" << std::endl;
    ofs << "</detector-grouping>" << std::endl;

    ofs.close();

    return;
  }

  /*
   * Test XML file using "ids" in old format
   */
  void test_OldFormat()
  {
    LoadDetectorsGroupingFile load;
    load.initialize();

    std::string xmlfilename = "testoldformat.xml";
    generateOldSpectrumIDXMLFile(xmlfilename);

    TS_ASSERT(load.setProperty("InputFile", xmlfilename));
    TS_ASSERT(load.setProperty("OutputWorkspace", "Random_Group_Old"));

    load.execute();
    TS_ASSERT(load.isExecuted());

    DataObjects::GroupingWorkspace_sptr gws =
        boost::dynamic_pointer_cast<DataObjects::GroupingWorkspace>(API::AnalysisDataService::Instance().retrieve("Random_Group_Old"));

    TS_ASSERT_DELTA(gws->dataY(0)[0], 1.0, 1.0E-5);
    TS_ASSERT_DELTA(gws->dataY(31)[0], 1.0, 1.0E-5);
    TS_ASSERT_DELTA(gws->dataY(32)[0], 2.0, 1.0E-5);
    TS_ASSERT_DELTA(gws->dataY(39)[0], 2.0, 1.0E-5);

    // Clean
    Poco::File cleanfile(xmlfilename);
    cleanfile.remove(false);

    return;
  }

  void generateOldSpectrumIDXMLFile(std::string xmlfilename)
  {
    std::ofstream ofs;
    ofs.open(xmlfilename.c_str(), std::ios::out);

    ofs << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << std::endl;
    ofs << "<detector-grouping>" << std::endl;
    ofs << "  <group name=\"fwd1\"> <ids val=\"1-32\"/> </group>" << std::endl;
    ofs << "  <group name=\"bwd1\"> <ids val=\"33,36,38,60-64\"/> </group>" << std::endl;
    ofs << "</detector-grouping>" << std::endl;

    ofs.close();

    return;
  }

};


#endif /* MANTID_DATAHANDLING_LOADDETECTORSGROUPINGFILETEST_H_ */
