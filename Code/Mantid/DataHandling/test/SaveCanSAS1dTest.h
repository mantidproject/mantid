#ifndef SAVECANSAS1DTEST_H
#define SAVECANSAS1DTEST_H
#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataHandling/SaveCanSAS1D.h"
#include "MantidDataHandling/LoadCanSAS1D.h"
#include "MantidKernel/UnitFactory.h"
#include "Poco/Path.h"

#include <fstream>
#include <sstream>

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class SaveCanSAS1DTest : public CxxTest::TestSuite
{
public:

  //set up the workspace that will be loaded
  SaveCanSAS1DTest() : m_workspace1("SaveCanSAS1DTest_in1"),
    m_workspace2("SaveCanSAS1DTest_in2"),
    m_filename("./savecansas1d.xml")

  {
    LoadRaw3 loader;
    if ( !loader.isInitialized() ) loader.initialize();
    std::string inputFile; // Path to test input file assumes Test directory checked out from SVN
    
    //the file's run number needs to be stored in m_runNum for later tests
    inputFile = Poco::Path(Poco::Path::current()).resolve("../../../../Test/AutoTestData/HET15869.raw").toString();
    m_runNum = "15869";

    loader.setPropertyValue("Filename", inputFile);
    loader.setPropertyValue("OutputWorkspace", m_workspace1);
    loader.setPropertyValue("SpectrumList", "1");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );

    // Change the unit to Q
    ws = boost::dynamic_pointer_cast<MatrixWorkspace>
      (AnalysisDataService::Instance().retrieve(m_workspace1));
    ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("MomentumTransfer");

    WorkspaceGroup_sptr group(new WorkspaceGroup);
    AnalysisDataService::Instance().addOrReplace("SaveCanSAS1DTest_group", group);
   // group->add("SaveCanSAS1DTest_group");
    group->add(m_workspace1);

    LoadRaw3 load;
    load.initialize();
    load.setPropertyValue("Filename", "../../../../Test/AutoTestData/IRS26173.raw");
    load.setPropertyValue("OutputWorkspace", m_workspace2);
    load.setPropertyValue("SpectrumList", "30");
    TS_ASSERT_THROWS_NOTHING(load.execute());
    TS_ASSERT( load.isExecuted() );

    // Change the unit to Q
    ws = boost::dynamic_pointer_cast<MatrixWorkspace>
      (AnalysisDataService::Instance().retrieve(m_workspace2));
    ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("MomentumTransfer");

    group->add(m_workspace2);
  }
//  //saving is required by all the following test so, if this test fails so will all the others!
//  void testExecute()
//  {
//    SaveCanSAS1D savealg;
//
//    TS_ASSERT_THROWS_NOTHING( savealg.initialize() );
//    TS_ASSERT( savealg.isInitialized() );
//    savealg.setPropertyValue("InputWorkspace", m_workspace1);
//    savealg.setPropertyValue("Filename", m_filename);
//    TS_ASSERT_THROWS_NOTHING(savealg.execute());
//    TS_ASSERT( savealg.isExecuted() );
//  }
//
//  void testCanSAS1dXML()
//  {
//    this->testExecute();
//
//    // read the generated xml file and compare first few lines of the file
//    std::ifstream testFile(m_filename.c_str(), std::ios::in);
//    TS_ASSERT ( testFile );
//    if (!testFile)
//      return;
//
//    //testing the first few lines of the xml file
//    std::string fileLine;
//    std::getline( testFile, fileLine );
//    std::getline( testFile, fileLine );
//
//    std::getline( testFile, fileLine );
//    std::string sasRootexpected=fileLine;
//
//    std::getline( testFile, fileLine );
//    sasRootexpected+=fileLine;
//    std::getline( testFile, fileLine );
//    sasRootexpected+=fileLine;
//
//    std::getline( testFile, fileLine );
//    sasRootexpected+=fileLine;
//
//    std::string sasroot;
//    sasroot="<SASroot version=\"1.0\"";
//    sasroot +="\t\t xmlns=\"cansas1d/1.0\"";
//    sasroot+="\t\t xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"";
//    sasroot+="\t\t xsi:schemaLocation=\"cansas1d/1.0 http://svn.smallangles.net/svn/canSAS/1dwg/trunk/cansas1d.xsd\">";
//    TS_ASSERT_EQUALS (sasroot,sasRootexpected);
//
//    std::getline( testFile, fileLine );
//    {
//      std::ostringstream correctLine;
//      correctLine << "\t<SASentry name=\"" << m_workspace1 << "\">";
//      TS_ASSERT_EQUALS ( fileLine, correctLine.str());
//    }
//
//    std::getline( testFile, fileLine );
//    TS_ASSERT_EQUALS ( fileLine, "\t\t<Title>White Van                             JAWS 45X45                                </Title>");
//    std::getline( testFile, fileLine );
//    {
//      std::ostringstream correctLine;
//      correctLine << "\t\t<Run>" << m_runNum << "</Run>";
//      TS_ASSERT_EQUALS ( fileLine, correctLine.str());
//    }
//
//    std::getline( testFile, fileLine );
//    TS_ASSERT_EQUALS ( fileLine,"\t\t<SASdata>");
//
//    std::getline( testFile, fileLine );
//    std::string idataline="\t\t\t<Idata><Q unit=\"1/A\">5.125</Q><I unit=\"Counts\">0</I><Idev unit=\"Counts\">0</Idev></Idata>";
//    TS_ASSERT_EQUALS ( fileLine,idataline);
//
//    testFile.close();
//
//
//
//    //no more tests on the file are possible after this
//    remove(m_filename.c_str());
//  }
//
//  void testGroup()
//  {
//    //do the save, the results of which we'll test
//    SaveCanSAS1D savealg;
//    TS_ASSERT_THROWS_NOTHING( savealg.initialize() );
//    TS_ASSERT( savealg.isInitialized() );
//    savealg.setPropertyValue("InputWorkspace", "SaveCanSAS1DTest_group");
//    savealg.setPropertyValue("Filename", m_filename);
//    TS_ASSERT_THROWS_NOTHING(savealg.execute());
//    TS_ASSERT( savealg.isExecuted() );
//
//    //retrieve the data that we saved to check it
//    LoadCanSAS1D lAlg;
//    TS_ASSERT_THROWS_NOTHING( lAlg.initialize() );
//    TS_ASSERT( lAlg.isInitialized() );
//    lAlg.setPropertyValue("OutputWorkspace", "newgroup");
//    lAlg.setPropertyValue("Filename", m_filename);
//    TS_ASSERT_THROWS_NOTHING(lAlg.execute());
//    TS_ASSERT( lAlg.isExecuted() );
//    Workspace_sptr ws = AnalysisDataService::Instance().retrieve("newgroup");
//    WorkspaceGroup_sptr group = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);
//
//    //we have the data now the tests begin
//    TS_ASSERT(group);
//    if (!group) return; //To avoid breaking the rest of the test runner.
//    std::vector<std::string> wNames = group->getNames();
//
//    TS_ASSERT_EQUALS(wNames.size(), 2); //change this and the lines below when group workspace names change
//    TS_ASSERT_EQUALS(wNames[0], m_workspace1);
//    TS_ASSERT_EQUALS(wNames[1], m_workspace2);
//
//    //check the second workspace in more detail
//    ws = Mantid::API::AnalysisDataService::Instance().retrieve(wNames[1]);
//    Mantid::DataObjects::Workspace2D_sptr ws2d = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws);
//    TS_ASSERT(ws2d);
//
//    Run run = ws2d->run();
//    Mantid::Kernel::Property *logP =run.getLogData("run_number");
//    TS_ASSERT_EQUALS( logP->value(), "26173");
//    TS_ASSERT_EQUALS(ws2d->getInstrument()->getName(), "IRIS");
//
//    TS_ASSERT_EQUALS( ws2d->getNumberHistograms(), 1 );
//    TS_ASSERT_EQUALS( ws2d->dataX(0).size(), 2000 );
//
//    //some of the data is only stored to 3 decimal places
//    double tolerance(1e-04);
//    TS_ASSERT_DELTA( ws2d->dataX(0).front(), 56005, tolerance );
//    TS_ASSERT_DELTA( ws2d->dataX(0)[1000], 66005, tolerance );
//    TS_ASSERT_DELTA( ws2d->dataX(0).back(), 75995, tolerance );
//
//    TS_ASSERT_DELTA( ws2d->dataY(0).front(), 0, tolerance );
//    TS_ASSERT_DELTA( ws2d->dataY(0)[1000], 1.0, tolerance );
//    TS_ASSERT_DELTA( ws2d->dataY(0).back(), 0, tolerance );
//
//    TS_ASSERT_DELTA( ws2d->dataE(0).front(), 0, tolerance );
//    TS_ASSERT_DELTA( ws2d->dataE(0)[1000], 1.0, tolerance );
//    TS_ASSERT_DELTA( ws2d->dataE(0).back(), 0, tolerance );
//  }



private :
  const std::string m_workspace1, m_workspace2, m_filename;
  std::string m_runNum;
  MatrixWorkspace_sptr ws;
};


#endif
