#include <cxxtest/TestSuite.h>
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include <boost/scoped_ptr.hpp>

using namespace Mantid;
using namespace Geometry;
class testMDGeometryDescription: public CxxTest::TestSuite
{
  MDGeometryDescription *pSlice;

  //helper constructional method.
  MDGeometryDescription* constructDescription()
  {
    std::vector<MDDimension> dimensions;
    MDDimension dimX("q1");
    MDDimension dimY("q2");
    MDDimension dimZ("q3");
    MDDimension dimt("p");
    MDDimension dimTemp("T");

    dimensions.push_back(dimX);
    dimensions.push_back(dimY);
    dimensions.push_back(dimZ);
    dimensions.push_back(dimt);
    dimensions.push_back(dimTemp);
    return new MDGeometryDescription(dimensions, dimX, dimY, dimZ, dimt);
  }
  ;

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
        "The constructor has not provided the alignment correctly. The dimension should have appeared in the first position.",
        ids[1], "q2");
  }

  void testAlignZ()
  {
    boost::scoped_ptr<MDGeometryDescription> description(constructDescription());
    std::vector<std::string> ids = description->getDimensionsTags();
    TSM_ASSERT_EQUALS(
        "The constructor has not provided the alignment correctly. The dimension should have appeared in the first position.",
        ids[2], "q3");
  }

  void testAlignt()
  {
    boost::scoped_ptr<MDGeometryDescription> description(constructDescription());
    std::vector<std::string> ids = description->getDimensionsTags();
    TSM_ASSERT_EQUALS(
        "The constructor has not provided the alignment correctly. The dimension should have appeared in the first position.",
        ids[3], "p");
  }

  void testMDGDconstructor()
  {
    TS_ASSERT_THROWS_NOTHING(pSlice = new MDGeometryDescription());
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
    TS_ASSERT_THROWS_NOTHING(pDescr->setNumBins("q1", 100));
    TS_ASSERT_THROWS_NOTHING(pDescr->setNumBins("q2", 100));
    TS_ASSERT_THROWS_NOTHING(pDescr->setNumBins("T", 100));

    TSM_ASSERT_EQUALS("The image size described by this description differs from expected", 100 * 100
        * 100, pDescr->getImageSize());
  }

  testMDGeometryDescription() :
    pSlice(NULL)
  {
  }
  ~testMDGeometryDescription()
  {
    if (pSlice)
      delete pSlice;
  }
};
