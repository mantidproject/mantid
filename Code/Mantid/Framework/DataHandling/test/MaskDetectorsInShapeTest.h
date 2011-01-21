#ifndef MARKDEADDETECTORSINSHAPETEST_H_
#define MARKDEADDETECTORSINSHAPETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/MaskDetectorsInShape.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidKernel/ArrayProperty.h"

class MaskDetectorsInShapeTest : public CxxTest::TestSuite
{
public:
  static MaskDetectorsInShapeTest *createSuite() { return new MaskDetectorsInShapeTest(); }
  static void destroySuite(MaskDetectorsInShapeTest *suite) { delete suite; }

  MaskDetectorsInShapeTest()
  {
    loadTestWS();
  }

  ~MaskDetectorsInShapeTest()
  {
    Mantid::API::AnalysisDataService::Instance().clear();
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
    using namespace Mantid::API;

    Mantid::DataHandling::MaskDetectorsInShape alg;
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

    MatrixWorkspace_const_sptr outWS =
        boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName));

    checkDeadDetectors(outWS, expectedHits);
  }

  void checkDeadDetectors(Mantid::API::MatrixWorkspace_const_sptr outWS,std::string expectedHits)
  {
    //check that the detectors have actually been marked dead
    std::vector<int> expectedDetectorArray = convertStringToVector(expectedHits);
    Mantid::Geometry::IInstrument_const_sptr i = outWS->getInstrument();
    for (std::vector<int>::iterator it = expectedDetectorArray.begin(); it!=expectedDetectorArray.end(); ++it)
    {
      TS_ASSERT( i->getDetector((*it))->isMasked() )
    }
  }

  std::vector<int> convertStringToVector(const std::string input)
  {
    Mantid::Kernel::ArrayProperty<int> arrayProp("name",input);
    return arrayProp();
  }

  std::string loadTestWS()
  {
    Mantid::DataHandling::LoadEmptyInstrument loaderSLS;

    TS_ASSERT_THROWS_NOTHING(loaderSLS.initialize());
    TS_ASSERT( loaderSLS.isInitialized() );
    inputFile = "SANDALS_Definition.xml";
    loaderSLS.setPropertyValue("Filename", inputFile);
    wsName = "MaskDetectorsInShapeTest_MaskDetectorsInShapeTestSLS";
    loaderSLS.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loaderSLS.execute());
    TS_ASSERT( loaderSLS.isExecuted() );

    return wsName;
  }


private:
  std::string inputFile;
  std::string wsName;

};

#endif /*MARKDEADDETECTORSINSHAPETEST_H_*/
