#include <cxxtest/TestSuite.h>
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

using namespace Mantid;
using namespace Geometry;
class MDGeometryDescriptionTest: public CxxTest::TestSuite
{
  MDGeometryDescription *pSlice;

  //helper constructional method.
  MDGeometryDescription* constructDescription()
  {
    std::vector<boost::shared_ptr<IMDDimension> > dimensions;
    boost::shared_ptr<IMDDimension> dimX = boost::shared_ptr<MDDimension>(new MDDimension("q1"));
    boost::shared_ptr<IMDDimension> dimY = boost::shared_ptr<MDDimension>(new MDDimension("q2"));
    boost::shared_ptr<IMDDimension> dimZ = boost::shared_ptr<MDDimension>(new MDDimension("q3"));
    boost::shared_ptr<IMDDimension> dimt = boost::shared_ptr<MDDimension>(new MDDimension("p"));
    boost::shared_ptr<IMDDimension> dimTemp = boost::shared_ptr<MDDimension>(new MDDimension("T"));

    dimensions.push_back(dimX);
    dimensions.push_back(dimY);
    dimensions.push_back(dimZ);
    dimensions.push_back(dimt);
    dimensions.push_back(dimTemp);
    RotationMatrix rotationMatrix(9,0);
    rotationMatrix[0] = rotationMatrix[4] = rotationMatrix[8] = 1; //Setup identity matrix for.
    return new MDGeometryDescription(dimensions, dimX, dimY, dimZ, dimTemp, rotationMatrix);
  }
  

public:

  void testAlignX()
  {
    boost::scoped_ptr<MDGeometryDescription> description(constructDescription());
    std::vector<std::string> ids = description->getDimensionsTags();
    TSM_ASSERT_EQUALS(
        "The constructor has not provided the alignment correctly. The dimension should have appeared in the first position.",
        ids[0], "q1");
  }

  void testAlignY()
  {
    boost::scoped_ptr<MDGeometryDescription> description(constructDescription());
    std::vector<std::string> ids = description->getDimensionsTags();
    TSM_ASSERT_EQUALS(
        "The constructor has not provided the alignment correctly. The dimension should have appeared in the second position.",
        ids[1], "q2");
  }

  void testAlignZ()
  {
    boost::scoped_ptr<MDGeometryDescription> description(constructDescription());
    std::vector<std::string> ids = description->getDimensionsTags();
    TSM_ASSERT_EQUALS(
        "The constructor has not provided the alignment correctly. The dimension should have appeared in the third position.",
        ids[2], "q3");
  }

  void testAlignt()
  {
    boost::scoped_ptr<MDGeometryDescription> description(constructDescription());
    std::vector<std::string> ids = description->getDimensionsTags();
    TSM_ASSERT_EQUALS(
        "The constructor has not provided the alignment correctly. The dimension should have appeared in the first position.",
        ids[3], "T");
  }

  void testMDGDconstructor()
  {
    TS_ASSERT_THROWS_NOTHING(pSlice = new MDGeometryDescription());
  }

void testMDGDDefaultConstructor(){
	std::auto_ptr<MDGeometryDescription> pDescr;
	TSM_ASSERT_THROWS_NOTHING("The default constructor should not throw",pDescr=std::auto_ptr<MDGeometryDescription>(new MDGeometryDescription)) ;
	MantidMat rot = pDescr->getRotations();
	TSM_ASSERT_EQUALS("default rotation matrix should me unit matrix",true,rot.equals(MantidMat(3,3,true),FLT_EPSILON));
}
  // these functions has not been written yet
  void testMDGDInput()
  {
    std::string input("");
    TS_ASSERT_EQUALS(pSlice->fromXMLstring(input), true);
  }

  void testMDGDOutput()
  {
    std::string output("");
    TS_ASSERT_THROWS_NOTHING(output = pSlice->toXMLstring());
    TS_ASSERT_EQUALS(output, "TEST PROPERTY");
  }
  void testDataSize()
  {
    boost::shared_ptr<MDGeometryDescription> pDescr = boost::shared_ptr<MDGeometryDescription>(
        constructDescription());
	TS_ASSERT_THROWS_NOTHING(pDescr->pDimDescription("q1")->nBins=100);
	TS_ASSERT_THROWS_NOTHING(pDescr->pDimDescription("q2")->nBins=100);
	TS_ASSERT_THROWS_NOTHING(pDescr->pDimDescription("T")->nBins =100);

    TSM_ASSERT_EQUALS("The image size described by this description differs from expected", 100 * 100
        * 100, pDescr->getImageSize());
  }
 


  MDGeometryDescriptionTest() :
    pSlice(NULL)
  {
  }
  ~MDGeometryDescriptionTest()
  {
    if (pSlice)
      delete pSlice;
  }
};
