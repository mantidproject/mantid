#ifndef SAVECANSAS1DTEST_H
#define SAVECANSAS1DTEST_H
#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataHandling/SaveCanSAS1D.h"
#include "MantidKernel/UnitFactory.h"
#include "Poco/Path.h"

#include <fstream>

class SaveCAnSAS1DTest : public CxxTest::TestSuite
{
public:

  void testCanSAS1dXML()
  {
    using namespace Mantid::DataHandling;
    using namespace Mantid::API;
	
	/*std::string s;
	std::getline(std::cin,s);*/

    SaveCanSAS1D savealg;
    LoadRaw3 loader;
    if ( !loader.isInitialized() ) loader.initialize();
    std::string inputFile; // Path to test input file assumes Test directory checked out from SVN
    inputFile = Poco::Path(Poco::Path::current()).resolve("../../../../Test/Data/HET15869.RAW").toString();

    std::string rawoutws="outWS";
    loader.setPropertyValue("Filename", inputFile);
    loader.setPropertyValue("OutputWorkspace", rawoutws);
    loader.setPropertyValue("SpectrumList", "1");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );

    // Change the unit to Q
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>
                                (AnalysisDataService::Instance().retrieve(rawoutws));
    ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("MomentumTransfer");

    TS_ASSERT_THROWS_NOTHING( savealg.initialize() )
    TS_ASSERT( savealg.isInitialized() )
    const std::string filename ="../../../../Test/Data/savecansas1d.xml";
    savealg.setPropertyValue("InputWorkspace", rawoutws);
    savealg.setPropertyValue("Filename", filename);
    TS_ASSERT_THROWS_NOTHING(savealg.execute());
    TS_ASSERT( savealg.isExecuted() );

    // read the generated xml file and compare first few lines of the file
    std::ifstream testFile(filename.c_str(), std::ios::in);
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
    TS_ASSERT_EQUALS ( fileLine,"\t<SASentry name=\"workspace\">");

    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine,"\t\t<Title>15869 DTA RIB              White Van                13-APR-2005 14:10:46</Title>");

    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine,"\t\t<Run>outWS</Run>");

    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine,"\t\t<SASdata>");

    std::getline( testFile, fileLine );
	std::string idataline="\t\t\t<Idata><Q unit=\"1/A\">5.125</Q><I unit=\"Counts\">0</I><Idev unit=\"Counts\">0</Idev></Idata>";
	TS_ASSERT_EQUALS ( fileLine,idataline);

  }
  
};


#endif
