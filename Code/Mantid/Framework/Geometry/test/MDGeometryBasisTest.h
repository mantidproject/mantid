#ifndef H_TEST_WORKSPACE_GEOMETRY
#define H_TEST_WORKSPACE_GEOMETRY

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/MDGeometry/MDGeometryBasis.h"
#include <set>
#include <cfloat>
#include <sstream>
#include <boost/scoped_ptr.hpp>

using namespace Mantid;
using namespace Geometry;


class MDGeometryBasisTest :   public CxxTest::TestSuite
{
private:

  //Helper method to construct a MDGeometryBasis schenario.
  static MDGeometryBasis* constructMDGeometryBasis()
  {
    using namespace Mantid::Geometry;
    std::set<MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("qx", true, 1));
    basisDimensions.insert(MDBasisDimension("qy", true, 2));
    basisDimensions.insert(MDBasisDimension("qz", true, 4));
    basisDimensions.insert(MDBasisDimension("p", false, 5));

    UnitCell cell;
    return new MDGeometryBasis(basisDimensions, cell);
  }

public:

  void testConstructionWithDuplicateColumnsThrows()
  {
    using namespace Mantid::Geometry;
    std::set<MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("qx", true, 1));
    basisDimensions.insert(MDBasisDimension("qy", true, 1));

    UnitCell cell;
    TSM_ASSERT_THROWS("Duplicate column numbers were used. Should have thrown.", MDGeometryBasis(basisDimensions, cell), std::logic_error);
  }

  void testGetReciprocalDimensions()
  {
    using namespace Mantid::Geometry;
    boost::scoped_ptr<MDGeometryBasis> basisDimension(constructMDGeometryBasis());
    std::set<MDBasisDimension> reciprocalDimensions = basisDimension->getReciprocalDimensions();
    TSM_ASSERT_LESS_THAN_EQUALS("Too many reciprocal dimensions.", 3, reciprocalDimensions.size());
    TSM_ASSERT("Expect to have at least 1 reciprocal dimension.", reciprocalDimensions.size() > 0);  
  }

  void testGetNonReciprocalDimensions()
  {
    using namespace Mantid::Geometry;
    boost::scoped_ptr<MDGeometryBasis> basisDimension(constructMDGeometryBasis());
    std::set<MDBasisDimension> nonReciprocalDimensions = basisDimension->getNonReciprocalDimensions();
    TSM_ASSERT_EQUALS("Wrong number of non-reciprocal dimensions returned.", 1, nonReciprocalDimensions.size());
  }

    void testGetAllBasisDimensions()
  {
    using namespace Mantid::Geometry;
    boost::scoped_ptr<MDGeometryBasis> basisDimension(constructMDGeometryBasis());
    std::set<MDBasisDimension> allBasisDimensions = basisDimension->getBasisDimensions();
    TSM_ASSERT_EQUALS("Wrong number of basis dimensions returned.", 4, allBasisDimensions.size());
  }

  
  void testConsistentNDimensions()
  {
    using namespace Mantid::Geometry;
    boost::scoped_ptr<MDGeometryBasis> basisDimension(constructMDGeometryBasis());
    std::set<MDBasisDimension> allBasisDimensions = basisDimension->getBasisDimensions(); 
    TSM_ASSERT_EQUALS("The number of dimensions returned via the getter should be the same as the actual number of dimensions present.", basisDimension->getNumDims(), allBasisDimensions.size());
  }

  void testTooManyDimensionsThrows()
  {
    std::set<MDBasisDimension> basisDimensions;

    std::stringstream stream;
    for(int i = 0; i < 22; i++)
    { 
      stream << i;
      basisDimensions.insert(MDBasisDimension(stream.str(), false, i));
    }
   
    UnitCell cell;
    TSM_ASSERT_THROWS("Cannot have this many basis dimensions.", MDGeometryBasis(basisDimensions, cell), std::invalid_argument);
  }

  void testTooManyReciprocalDimensionsThrows()
  {
    std::set<MDBasisDimension> basisDimensions;

    std::stringstream stream;
    for(int i = 0; i < 4; i++)
    { 
      stream << i;
      basisDimensions.insert(MDBasisDimension(stream.str(), true, i));
    }
   
    UnitCell cell;
    TSM_ASSERT_THROWS("Cannot have this many reciprocal basis dimensions.", MDGeometryBasis(basisDimensions, cell), std::invalid_argument);
  }

  void testIDCompartibility(){

    std::vector<std::string> new_ids(4,"");
    new_ids[0]="qx";
    new_ids[1]="qy";
    new_ids[2]="qz";
    new_ids[3]="p";

    boost::scoped_ptr<MDGeometryBasis> basis(constructMDGeometryBasis());
    TS_ASSERT_EQUALS(basis->checkIdCompartibility(new_ids),true);

    new_ids[0]="k"; //some unknown id value
    TS_ASSERT_EQUALS(basis->checkIdCompartibility(new_ids),false);
  }

  ~MDGeometryBasisTest()
  {
  }


};



#endif
