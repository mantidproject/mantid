#ifndef H_TEST_BAISI_DIMENSION
#define H_TEST_BAISI_DIMENSION

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/MDGeometry/MDGeometryBasis.h"
#include <set>
#include <cfloat>
#include <sstream>
#include <boost/scoped_ptr.hpp>

using namespace Mantid;
using namespace Geometry;

class MDBasisDimensionTest : public CxxTest::TestSuite
{
public:


	
void testConstrWithWrongDirectionThrows()
{
	std::auto_ptr<MDBasisDimension> pDim;
	TSM_ASSERT_THROWS("Basis reciprocal dimensions should throw on any columnt number except 0, 1, or 2",
		pDim = std::auto_ptr<MDBasisDimension>(new MDBasisDimension("kx",true,4)),std::invalid_argument);
}
void testConstrOrthoDimensionWithWrongDirectionThrows()
{
	std::auto_ptr<MDBasisDimension> pDim;
	TSM_ASSERT_THROWS("Orthogonal dimension with non-zero component throws, orthogonal direction has to be 0",
		pDim = std::auto_ptr<MDBasisDimension>(new MDBasisDimension("kx",false,4,"",V3D(1,1,0))),std::invalid_argument);
}
void testConstrRecoDimensionWithZeroLengthRedefines()
{
	std::auto_ptr<MDBasisDimension> pDim;
	TSM_ASSERT_THROWS_NOTHING("Reciprocal dimension with  zero length should not throw as it redefines the direction",
		pDim = std::auto_ptr<MDBasisDimension>(new MDBasisDimension("kx",true,1,"",V3D(0,0,0))));
	TSM_ASSERT_DELTA("Reciprocal dimension defined with 0 length redefined: ",1,pDim->getDirection().Y(),1e-6);

}

void testDefaultOrthogonalUnitsAreEnergy(){
	std::auto_ptr<MDBasisDimension> pDim = std::auto_ptr<MDBasisDimension>(new MDBasisDimension("en",false,4));
	TSM_ASSERT_EQUALS("Default orthogonal units should be energy transfer","DeltaE",pDim->getUnits().unitID());
}
void testDefaultOrthogonalLengthIsZero(){
	std::auto_ptr<MDBasisDimension> pDim = std::auto_ptr<MDBasisDimension>(new MDBasisDimension("en",false,4));
	TSM_ASSERT_DELTA("Default orthogonal length should be zero",0,pDim->getDirection().norm2(),1.e-6);
}

void testMDBDimDefaultUintsAreQ()
{
	std::auto_ptr<MDBasisDimension> pDim = std::auto_ptr<MDBasisDimension>(new MDBasisDimension("kx",true,0));
	TSM_ASSERT_EQUALS("Default rec-dim units should be the momentum transfer","MomentumTransfer",pDim->getUnits().unitID());

}
void testMDBDimDefaultLengthIsOneInX()
{
	std::auto_ptr<MDBasisDimension> pDim = std::auto_ptr<MDBasisDimension>(new MDBasisDimension("kx",true,0));
	TSM_ASSERT_DELTA("Default rec-dim length should be 1",1,pDim->getDirection().norm2(),1.e-6);
	TSM_ASSERT_DELTA("Default rec-dim should be directed in proper direction",1,pDim->getDirection().X(),1.e-6);

}
void testMDBDimDefaultLengthIsOneInY()
{
	std::auto_ptr<MDBasisDimension> pDim = std::auto_ptr<MDBasisDimension>(new MDBasisDimension("kx",true,1));
	TSM_ASSERT_DELTA("Default rec-dim length should be 1",1,pDim->getDirection().norm2(),1.e-6);
	TSM_ASSERT_DELTA("Default rec-dim should be directed in proper direction",1,pDim->getDirection().Y(),1.e-6);

}
void testMDBDimDefaultLengthIsOneInZ()
{
	std::auto_ptr<MDBasisDimension> pDim = std::auto_ptr<MDBasisDimension>(new MDBasisDimension("kx",true,2));
	TSM_ASSERT_DELTA("Default rec-dim length should be 1",1,pDim->getDirection().norm2(),1.e-6);
	TSM_ASSERT_DELTA("Default rec-dim should be directed in proper direction",1,pDim->getDirection().Z(),1.e-6);

}



};
#endif