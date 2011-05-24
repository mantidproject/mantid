#ifndef H_TEST_MDGEOMETRY_BASIS
#define H_TEST_MDGEOMETRY_BASIS

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

    basisDimensions.insert(MDBasisDimension("qx", true, 0,"",V3D(1,0,0)));
    basisDimensions.insert(MDBasisDimension("qz", true, 2,"",V3D(0,sqrt(2.)/2,0)));
    basisDimensions.insert(MDBasisDimension("qy", true, 1,"",V3D(0,0,sqrt(3.)/2)));
    basisDimensions.insert(MDBasisDimension("p", false, 3));

	boost::shared_ptr<OrientedLattice> spCell = boost::shared_ptr<OrientedLattice>(new OrientedLattice(2.87,2.87,2.87));
    return new MDGeometryBasis(basisDimensions,spCell);
  }

public:

  void testConstructionWithDuplicateColumnsThrows()
  {
    using namespace Mantid::Geometry;
    std::set<MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("qx", true, 1));
    basisDimensions.insert(MDBasisDimension("qy", true, 1));

 	boost::shared_ptr<OrientedLattice> spCell = boost::shared_ptr<OrientedLattice>(new OrientedLattice(2.87,2.87,2.87));
    TSM_ASSERT_THROWS("Duplicate column numbers were used. Should have thrown.", MDGeometryBasis(basisDimensions,spCell), std::logic_error);
   }
  void testConstructionNonOrthogonalBasisThrows()
  {
    using namespace Mantid::Geometry;
    std::set<MDBasisDimension> basisDimensions;

    basisDimensions.insert(MDBasisDimension("qx", true, 0,"",V3D(1,0,0)));
    basisDimensions.insert(MDBasisDimension("qy", true, 1,"",V3D(1,1,0)));

 	boost::shared_ptr<OrientedLattice> spCell = boost::shared_ptr<OrientedLattice>(new OrientedLattice(2.87,2.87,2.87));
    TSM_ASSERT_THROWS("Non-orthogonal dimensions were used. Should have thrown.", MDGeometryBasis(basisDimensions,spCell), std::logic_error);
  }
 

   void testConstructWithWrongColumnNumbersThrows(){

    using namespace Mantid::Geometry;
    std::set<MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("qx", true, 1));
    basisDimensions.insert(MDBasisDimension("qy", true, 2));
 	boost::shared_ptr<OrientedLattice> spCell = boost::shared_ptr<OrientedLattice>(new OrientedLattice(2.87,2.87,2.87));
    TSM_ASSERT_THROWS("Number of any dimension has to be smaller then total number of dimensions. Should have thrown.", MDGeometryBasis(basisDimensions,spCell), std::invalid_argument);

  }

  void testGetReciprocalDimensions()
  {
    using namespace Mantid::Geometry;
    boost::scoped_ptr<MDGeometryBasis> basisDimension(constructMDGeometryBasis());
    std::set<MDBasisDimension> reciprocalDimensions = basisDimension->getReciprocalDimensions();
    TSM_ASSERT_LESS_THAN_EQUALS("Too many reciprocal dimensions.", 3, reciprocalDimensions.size());
    TSM_ASSERT("Expect to have at least 1 reciprocal dimension.", reciprocalDimensions.size() > 0);  
  }

  void testGetRecDimBasis(){
	  boost::scoped_ptr<MDGeometryBasis> basisDimension(constructMDGeometryBasis());
	  std::vector<V3D> basis = basisDimension->get_constRecBasis();
	  //TODO: Clarify: there is question, if we should allow geometry basis to have non-unit vectors; does it have any physical meaning?
	  TSM_ASSERT_EQUALS("first basis dimension in this case should be 1,0,0",true,basis[0]==V3D(1,0,0));
	  TSM_ASSERT_EQUALS("second basis dimension in this case should be 0,0,sqrt(3.)/2",true,basis[1]==V3D(0,0,sqrt(3.)/2));
	  TSM_ASSERT_EQUALS("Third basis dimension in this case should be 0,sqrt(2.)/2,0",true,basis[2]==V3D(0,sqrt(2.)/2,0));
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
   
	boost::shared_ptr<OrientedLattice> spCell = boost::shared_ptr<OrientedLattice>(new OrientedLattice(2.87,2.87,2.87));
    TSM_ASSERT_THROWS("Cannot have this many basis dimensions.", MDGeometryBasis(basisDimensions,spCell), std::invalid_argument);
  }

 /* This test currently disabled as you can not generage more then 3 reciprocal dimensions -- this will throw on MDBasisDimension
 void t__tTooManyReciprocalDimensionsThrows()
  {
    std::set<MDBasisDimension> basisDimensions;
	bool is_reciprocal;
    std::stringstream stream;
    for(int i = 0; i < 4; i++)
    { 
      stream << i;
	  if(i<3){
		  is_reciprocal=true;
	  }else{
		  is_reciprocal=false;
	  }
      basisDimensions.insert(MDBasisDimension(stream.str(), is_reciprocal, i));
    }
  	boost::shared_ptr<OrientedLattice> spCell = boost::shared_ptr<OrientedLattice>(new OrientedLattice(2.87,2.87,2.87));
    TSM_ASSERT_THROWS("Cannot have this many reciprocal basis dimensions.", MDGeometryBasis(basisDimensions,spCell), std::invalid_argument);
  }*/

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
  void testGetID(){
	 MDGeometryBasis* mdBasis = constructMDGeometryBasis();
	 std::vector<std::string> dimID;
	 TSM_ASSERT_THROWS_NOTHING("getBasisIDs is harmless and should not throw ",dimID=mdBasis->getBasisIDs());

	 TSM_ASSERT_EQUALS("4 dimensions should be constructed ",4,dimID.size());

     TSM_ASSERT_EQUALS("Each DIM_id obrained has to belong to dimensions",true,mdBasis->checkIdCompartibility(dimID));

  }
  ~MDGeometryBasisTest()
  {
  }


};



#endif
