#ifndef MARKDEADDETECTORSINSHAPETEST_H_
#define MARKDEADDETECTORSINSHAPETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/MaskDetectorsInShape.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ArrayProperty.h"
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

class MaskDetectorsInShapeTest : public CxxTest::TestSuite
{
public:

  MaskDetectorsInShapeTest()
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
		MaskDetectorsInShape alg;
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

		Workspace2D_sptr outWS = boost::dynamic_pointer_cast<Workspace2D>(AnalysisDataService::Instance().retrieve(wsName));

		checkDeadDetectors(outWS, expectedHits);
	}

	void checkDeadDetectors(Workspace2D_sptr outWS,std::string expectedHits)
	{
		//check that the detectors have actually been marked dead
		std::vector<int> expectedDetectorArray = convertStringToVector(expectedHits);
		boost::shared_ptr<IInstrument> i = outWS->getInstrument();
		for (std::vector<int>::iterator it = expectedDetectorArray.begin(); it!=expectedDetectorArray.end(); ++it) 
		{
			TS_ASSERT( i->getDetector((*it))->isMasked() )
		}
  }

	std::vector<int> convertStringToVector(const std::string input)
	{
		ArrayProperty<int> arrayProp("name",input);
		return arrayProp();
	}

	std::string loadTestWS()
	{
    LoadEmptyInstrument loaderSLS;

    TS_ASSERT_THROWS_NOTHING(loaderSLS.initialize());
    TS_ASSERT( loaderSLS.isInitialized() );
    inputFile = "../../../../Test/Instrument/SANDALS_Definition.xml";
    loaderSLS.setPropertyValue("Filename", inputFile);
    wsName = "MaskDetectorsInShapeTestSLS";
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

#endif /*MARKDEADDETECTORSINSHAPETEST_H_*/
