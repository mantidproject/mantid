#ifndef SAVECANSAS1DTEST_H
#define SAVECANSAS1DTEST_H
#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataHandling/SaveCanSAS1D.h"
#include "MantidKernel/UnitFactory.h"
#include "Poco/Path.h"

#include <fstream>
#include <sstream>

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class SaveCAnSAS1DTest : public CxxTest::TestSuite
{
public:

  //set up the workspace that will be loaded
  SaveCAnSAS1DTest() : m_rawoutws("SaveCAnSAS1DTest_inWS"),
    m_filename("../../../../Test/Data/savecansas1d.xml")

  {
    LoadRaw3 loader;
    if ( !loader.isInitialized() ) loader.initialize();
    std::string inputFile; // Path to test input file assumes Test directory checked out from SVN
    
    //the file's run number needs to be stored in m_runNum for later tests
    inputFile = Poco::Path(Poco::Path::current()).resolve("../../../../Test/Data/HET15869.RAW").toString();
    m_runNum = "15869";

    loader.setPropertyValue("Filename", inputFile);
    loader.setPropertyValue("OutputWorkspace", m_rawoutws);
    loader.setPropertyValue("SpectrumList", "1");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );

    // Change the unit to Q
    ws = boost::dynamic_pointer_cast<MatrixWorkspace>
      (AnalysisDataService::Instance().retrieve(m_rawoutws));
    ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("MomentumTransfer");

  }
  //saving is required by all the following test so, if this test fails so will all the others!
  void testExecute()
  {
    SaveCanSAS1D savealg;

    TS_ASSERT_THROWS_NOTHING( savealg.initialize() )
    TS_ASSERT( savealg.isInitialized() )
    savealg.setPropertyValue("InputWorkspace", m_rawoutws);
    savealg.setPropertyValue("Filename", m_filename);
    TS_ASSERT_THROWS_NOTHING(savealg.execute());
    TS_ASSERT( savealg.isExecuted() );
  }
  
  void testCanSAS1dXML()
  {
	  // read the generated xml file and compare first few lines of the file
    std::ifstream testFile(m_filename.c_str(), std::ios::in);
    TS_ASSERT ( testFile )

    //testing the first few lines of the xml file
    std::string fileLine;
    std::getline( testFile, fileLine );
	std::getline( testFile, fileLine );

	std::getline( testFile, fileLine );
	std::string sasRootexpected=fileLine;

    std::getline( testFile, fileLine );
	sasRootexpected+=fileLine;
    std::getline( testFile, fileLine );
	sasRootexpected+=fileLine;

	std::getline( testFile, fileLine );
	sasRootexpected+=fileLine;

    std::string sasroot;
    sasroot="<SASroot version=\"1.0\"";
	sasroot +="\t\t xmlns=\"cansas1d/1.0\"";
	sasroot+="\t\t xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"";
	sasroot+="\t\t xsi:schemaLocation=\"cansas1d/1.0 http://svn.smallangles.net/svn/canSAS/1dwg/trunk/cansas1d.xsd\">";
    TS_ASSERT_EQUALS (sasroot,sasRootexpected);

    std::getline( testFile, fileLine );
    {
      std::ostringstream correctLine;
      correctLine << "\t<SASentry name=\"" << m_rawoutws << "\">";
      TS_ASSERT_EQUALS ( fileLine, correctLine.str());
    }

    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine, "\t\t<Title>White Van                             JAWS 45X45                                </Title>")
    std::getline( testFile, fileLine );
    {
      std::ostringstream correctLine;
      correctLine << "\t\t<Run>" << m_runNum << "</Run>";
      TS_ASSERT_EQUALS ( fileLine, correctLine.str());
    }

    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine,"\t\t<SASdata>");

    std::getline( testFile, fileLine );
	std::string idataline="\t\t\t<Idata><Q unit=\"1/A\">5.125</Q><I unit=\"Counts\">0</I><Idev unit=\"Counts\">0</Idev></Idata>";
	TS_ASSERT_EQUALS ( fileLine,idataline);

    testFile.close();


  
    //no more tests on the file are possible after this
    remove(m_filename.c_str());
  }

private :
  const std::string m_rawoutws, m_filename;
  std::string m_runNum;
  MatrixWorkspace_sptr ws;
};


#endif
