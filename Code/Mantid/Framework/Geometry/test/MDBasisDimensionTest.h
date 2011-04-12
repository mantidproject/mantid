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
	TSM_ASSERT_THROWS("Orthogonal dimension with non-zero 1 or 2 component throws, only 0-compomnent has to be non-0",
		pDim = std::auto_ptr<MDBasisDimension>(new MDBasisDimension("kx",false,4,V3D(1,1,0))),std::invalid_argument);


}
void testDefaultOrthogonalUnitsAreEnergy(){
	std::auto_ptr<MDBasisDimension> pDim = std::auto_ptr<MDBasisDimension>(new MDBasisDimension("en",false,4));
	TSM_ASSERT_EQUALS("Default orthogonal units should be energy transfer","DeltaE",pDim->getUnits().unitID());
}
void testMDBDimDefaultUintsAreQ()
{
	std::auto_ptr<MDBasisDimension> pDim = std::auto_ptr<MDBasisDimension>(new MDBasisDimension("kx",true,0));
	TSM_ASSERT_EQUALS("Default rec-dim units should be the momentum transfer","MomentumTransfer",pDim->getUnits().unitID());

}

};
#endif