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

    SaveCanSAS1D savealg;
    LoadRaw3 loader;
    if ( !loader.isInitialized() ) loader.initialize();
    std::string inputFile; // Path to test input file assumes Test directory checked out from SVN
    inputFile = Poco::Path(Poco::Path::current()).resolve("../../../../Test/Data/LOQ48127.raw").toString();

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
    std::string sasroot;
    sasroot="<SASroot version=\"1.0\" xmlns=\"cansas1d/1.0\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"cansas1d/1.0 http://svn.smallangles.net/svn/canSAS/1dwg/trunk/cansas1d.xsd\">";
    
    TS_ASSERT_EQUALS (fileLine,sasroot);
    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine,"\t<SASentry>");

    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine,"\t\t<Title>48127 LOQ team &amp; SANS Xpre direct beam              18-DEC-2008 17:58:38</Title>");

    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine,"\t\t<Run>outWS</Run>");

    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine,"\t\t<SASdata>");

    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine,"\t\t\t<Idata>");
    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine,"\t\t\t\t<Q unit=\"1/A\">3543.75</Q>");

    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine,"\t\t\t\t<I unit=\"Counts\">111430</I>");

    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine,"\t\t\t\t<Idev unit=\"Counts\">333.811</Idev>");

}


};


#endif
