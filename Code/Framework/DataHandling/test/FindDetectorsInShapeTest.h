#ifndef FINDDETECTORSINSHAPETEST_H_
#define FINDDETECTORSINSHAPETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/FindDetectorsInShape.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidGeometry/Instrument/Component.h"
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class FindDetectorsInShapeTest : public CxxTest::TestSuite
{
public:

  FindDetectorsInShapeTest()
  {
		loadTestWS();
  }

	void testCuboidMiss()
	{
		std::string xmlShape = "<cuboid id=\"shape\"> ";
		xmlShape +=	"<left-front-bottom-point x=\"0.005\" y=\"-0.1\" z=\"0.0\" /> " ;
  	xmlShape +=	"<left-front-top-point x=\"0.005\" y=\"-0.1\" z=\"0.0001\" />  " ;
  	xmlShape +=	"<left-back-bottom-point x=\"-0.005\" y=\"-0.1\" z=\"0.0\" />  " ;
  	xmlShape +=	"<right-front-bottom-point x=\"0.005\" y=\"0.1\" z=\"0.0\" />  " ;
  	xmlShape +=	"</cuboid> ";
		xmlShape +=	"<algebra val=\"shape\" /> ";  
		
		runTest(xmlShape,"");
	}

	void testCuboidHit()
	{
		//algebra line is essential
		std::string xmlShape = "<cuboid id=\"shape\"> ";
		xmlShape +=	"<left-front-bottom-point x=\"0.674291\" y=\"0.335987\" z=\"1.30628\" /> " ;
  	xmlShape +=	"<left-front-top-point x=\"0.674291\" y=\"0.335987\" z=\"1.34783\" />  " ;
  	xmlShape +=	"<left-back-bottom-point x=\"0.673866\" y=\"0.335987\" z=\"1.30628\" />  " ;
  	xmlShape +=	"<right-front-bottom-point x=\"0.674291\" y=\"0.336189\" z=\"1.30628\" />  " ;
  	xmlShape +=	"</cuboid>";
		xmlShape +=	"<algebra val=\"shape\" /> ";  
		
		runTest(xmlShape,"977,978,1017,1018");
	}
	
	void testSphereMiss()
	{
		//algebra line is essential
		std::string xmlShape = "<sphere id=\"shape\"> ";
		xmlShape +=	"<centre x=\"4.1\"  y=\"2.1\" z=\"8.1\" /> " ;
  	xmlShape +=	"<radius val=\"3.2\" /> " ;
  	xmlShape +=	"</sphere>";
		xmlShape +=	"<algebra val=\"shape\" /> ";  

		runTest(xmlShape,"");
	}

	void testSphereHit()
	{
		//algebra line is essential
		std::string xmlShape = "<sphere id=\"shape\"> ";
		xmlShape +=	"<centre x=\"0.67\"  y=\"0.33\" z=\"1.32\" /> " ;
  	xmlShape +=	"<radius val=\"0.05\" /> " ;
  	xmlShape +=	"</sphere>";
		xmlShape +=	"<algebra val=\"shape\" /> ";  

		runTest(xmlShape,"976,977,978,979,980,1016,1017,1018,1019,1020");
	}

	void testCylinderHit()
	{
		//algebra line is essential
		std::string xmlShape = "<cylinder id=\"shape\"> ";
		xmlShape +=	"<centre-of-bottom-base x=\"0.0\" y=\"0.0\" z=\"0.0\" /> " ; 
  	xmlShape +=	"<axis x=\"0.0\" y=\"0.0\" z=\"1\" /> " ;
  	xmlShape +=	"<radius val=\"0.1\" /> " ;
  	xmlShape +=	"<height val=\"3\" /> " ;
  	xmlShape +=	"</cylinder>";
		xmlShape +=	"<algebra val=\"shape\" /> ";  

		runTest(xmlShape,"1,3");
	}

	void testInfiniteCylinderHit()
	{
		//algebra line is essential
		std::string xmlShape = "<infinite-cylinder id=\"shape\"> ";
		xmlShape +=	"<centre x=\"0.0\" y=\"0.0\" z=\"0.0\" /> " ; 
  	xmlShape +=	"<axis x=\"0.0\" y=\"0.0\" z=\"1\" /> " ;
  	xmlShape +=	"<radius val=\"0.1\" /> " ;
  	xmlShape +=	"</infinite-cylinder>";
		xmlShape +=	"<algebra val=\"shape\" /> ";  

		runTest(xmlShape,"1,2,3");
	}

	void testConeHitNoMonitors()
	{
		//algebra line is essential
		std::string xmlShape = "<cone id=\"shape\"> ";
		xmlShape +=	"<tip-point x=\"0.0\" y=\"0.0\" z=\"0.0\" /> " ; 
  	xmlShape +=	"<axis x=\"0.0\" y=\"0.0\" z=\"-1\" /> " ;
  	xmlShape +=	"<angle val=\"8.1\" /> " ;
  	xmlShape +=	"<height val=\"4\" /> " ;
  	xmlShape +=	"</cone>";
		xmlShape +=	"<algebra val=\"shape\" /> ";  

		runTest(xmlShape,"320,340,360,380",false);
	}

	void runTest(std::string xmlShape, std::string expectedHits,bool includeMonitors = true)
  {
		FindDetectorsInShape alg;
		TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    alg.setPropertyValue("Workspace", wsName);
    alg.setPropertyValue("ShapeXML", xmlShape);
		if (includeMonitors)
		{
			alg.setPropertyValue("IncludeMonitors", "1");
		}
    
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT( alg.isExecuted() );

		TS_ASSERT_EQUALS(alg.getPropertyValue("DetectorList"),expectedHits);
  }

	std::string loadTestWS()
	{
    LoadEmptyInstrument loaderSLS;

    TS_ASSERT_THROWS_NOTHING(loaderSLS.initialize());
    TS_ASSERT( loaderSLS.isInitialized() );
    inputFile = "../../../../Test/Instrument/SANDALS_Definition.xml";
    loaderSLS.setPropertyValue("Filename", inputFile);
    wsName = "FindDetectorsInShapeTestSLS";
    loaderSLS.setPropertyValue("OutputWorkspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderSLS.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderSLS.getPropertyValue("OutputWorkspace") )
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderSLS.execute());

    TS_ASSERT( loaderSLS.isExecuted() );


    MatrixWorkspace_sptr output;
    output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName));
    // Check the total number of elements in the map for SLS
    TS_ASSERT_EQUALS(output->spectraMap().nElements(),683);
	
		return wsName;
	}


private:
  std::string inputFile;
  std::string wsName;

};

#endif /*FINDDETECTORSINSHAPETEST_H_*/
