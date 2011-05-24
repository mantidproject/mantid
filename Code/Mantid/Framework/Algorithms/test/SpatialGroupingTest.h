#ifndef SPATIALGROUPINGTEST_H_
#define SPATIALGROUPINGTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/SpatialGrouping.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/ConfigService.h"

#include <Poco/Path.h>
#include <Poco/File.h>

#include "MantidDataHandling/LoadRaw3.h"


class SpatialGroupingTest : public CxxTest::TestSuite
{
public:
  SpatialGroupingTest() {}
  ~SpatialGroupingTest() {}

  void testWISH()
  {
    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setChild(true);
    loader.setPropertyValue("Filename", "/home/dmn58364/mantidproject/svn/trunk/Test/Data/WISH00016748.raw");
    loader.setPropertyValue("OutputWorkspace", "wish");
    loader.execute();

    Mantid::API::Workspace_sptr ws = loader.getProperty("OutputWorkspace");
    Mantid::API::MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws);

    outputWS->getDetector(100)->getNeighbours();

  }

  void testMetaInfo()
  {
    alg = new Mantid::Algorithms::SpatialGrouping();
    TS_ASSERT_EQUALS(alg->name(), "SpatialGrouping");
    TS_ASSERT_EQUALS(alg->version(), 1);
    TS_ASSERT_EQUALS(alg->category(), "General");
    delete alg;
  }

  void testInit()
  {
    alg = new Mantid::Algorithms::SpatialGrouping();
    TS_ASSERT(! alg->isInitialized() );
    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    TS_ASSERT(alg->isInitialized());
    delete alg;
  }

  void testExec()
  {
    // Test the algorithm
    // Create a workspace
    const int nhist(18);
    Mantid::API::MatrixWorkspace_sptr workspace = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(WorkspaceCreationHelper::Create2DWorkspaceBinned(18, 1));
    // Create a parameterised instrument
    Mantid::Geometry::Instrument_sptr instrument = boost::dynamic_pointer_cast<Mantid::Geometry::Instrument>(ComponentCreationHelper::createTestInstrumentCylindrical(2));
    for( int i = 1; i < nhist + 1; ++i )
    {
      workspace->getAxis(1)->spectraNo(i-1) = i;
    }
    workspace->mutableSpectraMap().populateSimple(1, nhist+1);
    workspace->setInstrument(instrument);

    alg = new Mantid::Algorithms::SpatialGrouping();
    alg->initialize();
    alg->setProperty<Mantid::API::MatrixWorkspace_sptr>("InputWorkspace", workspace);
    alg->setProperty("Filename", "test_SpatialGrouping");
    alg->execute();

    TS_ASSERT(alg->isExecuted());

    std::string file = alg->getProperty("Filename");

    Poco::File fileobj(file);
    const bool fileExists = fileobj.exists();

    TSM_ASSERT(fileobj.path(),fileExists);
    
    std::ifstream input;
    input.open(file.c_str());

    /* output should match:

    <?xml version="1.0" encoding="UTF-8" ?>
    <!-- XML Grouping File created by SpatialGrouping Algorithm -->
    <detector-grouping>
    <group name="group1"><detids val="1,2,3,4,5,6,7,8,9"/></group>
    <group name="group2"><detids val="10,11,12,13,14,15,16,17,18"/></group>
    </detector-grouping>
    
    */

    TS_ASSERT( input.is_open() );

    // Testing file
    std::vector<std::string> expected;
    expected.push_back("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>");
    expected.push_back("<!-- XML Grouping File created by SpatialGrouping Algorithm -->");
    expected.push_back("<detector-grouping>");
    expected.push_back("<group name=\"group1\"><detids val=\"1,2,3,4,5,6,7,8,9\"/></group>");
    expected.push_back("<group name=\"group2\"><detids val=\"10,11,12,13,14,15,16,17,18\"/></group>");
    expected.push_back("</detector-grouping>");

    std::vector<std::string>::iterator iter = expected.begin();

    while ( input.good() && iter != expected.end() )
    {
      std::string received;
      getline(input, received);

      TS_ASSERT_EQUALS(received, *iter);
      ++iter;
    }

    // close file
    input.close();

    // delete file
    remove(file.c_str());
  }
private:
  Mantid::Algorithms::SpatialGrouping* alg;

};
#endif
