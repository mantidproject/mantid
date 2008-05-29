#ifndef MANTID_TESTSURFACEFACTORY__
#define MANTID_TESTSURFACEFACTORY__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <ostream>
#include <vector>
#include <algorithm>
#include <sstream>

#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"

#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Quadratic.h"
#include "MantidGeometry/Plane.h"
#include "MantidGeometry/Sphere.h"
#include "MantidGeometry/Cone.h"
#include "MantidGeometry/Cylinder.h"
#include "MantidGeometry/Torus.h"
#include "MantidGeometry/SurfaceFactory.h"


using namespace Mantid;
using namespace Geometry;

class testSurfaceFactory: public CxxTest::TestSuite
{
public:
	void testCreateSurface(){
		SurfaceFactory *A;
		A=SurfaceFactory::Instance();
		Plane *P =dynamic_cast<Plane*>(A->createSurface("Plane"));
		TS_ASSERT_EQUALS(extractString(*P),"-1 px 0\n");
		Sphere *B=dynamic_cast<Sphere*>(A->createSurface("Sphere"));
		TS_ASSERT_EQUALS(extractString(*B),"-1 so 0\n");
		Cylinder *C=dynamic_cast<Cylinder*>(A->createSurface("Cylinder"));
		TS_ASSERT_EQUALS(extractString(*C),"-1 cx 0\n");
		Cone     *K=dynamic_cast<Cone*>(A->createSurface("Cone"));
		TS_ASSERT_EQUALS(extractString(*K),"-1  kx 0 0\n");
		Torus    *T=dynamic_cast<Torus*>(A->createSurface("Torus"));
		TS_ASSERT_EQUALS(extractString(*T),"-1 tx [0,0,0] 0 0 0\n");		
	}

	void testCreateSurfaceID(){
		SurfaceFactory *A;
		A=SurfaceFactory::Instance();
		Plane *P =dynamic_cast<Plane*>(A->createSurfaceID("p"));
		TS_ASSERT_EQUALS(extractString(*P),"-1 px 0\n");
		Sphere *B=dynamic_cast<Sphere*>(A->createSurfaceID("s"));
		TS_ASSERT_EQUALS(extractString(*B),"-1 so 0\n");
		Cylinder *C=dynamic_cast<Cylinder*>(A->createSurfaceID("c"));
		TS_ASSERT_EQUALS(extractString(*C),"-1 cx 0\n");
		Cone     *K=dynamic_cast<Cone*>(A->createSurfaceID("k"));
		TS_ASSERT_EQUALS(extractString(*K),"-1  kx 0 0\n");
		Torus    *T=dynamic_cast<Torus*>(A->createSurfaceID("t"));
		TS_ASSERT_EQUALS(extractString(*T),"-1 tx [0,0,0] 0 0 0\n");
	}

	void testProcessLine(){
		SurfaceFactory *A;
		A=SurfaceFactory::Instance();
		Surface *P=A->processLine("pz 5");
		Plane tP;
		tP.setSurface("pz 5");
		TS_ASSERT_EQUALS(extractString(*P),extractString(tP));
		Surface *S=A->processLine("s 1.1 -2.1 1.1 2");
		Sphere tS;
		tS.setSurface("s 1.1 -2.1 1.1 2");
		TS_ASSERT_EQUALS(extractString(*S),extractString(tS));
		Surface *C=A->processLine("c/x 0.5 0.5 1.0");
		Cylinder tC;
		tC.setSurface("c/x 0.5 0.5 1.0");
		TS_ASSERT_EQUALS(extractString(*C),extractString(tC));
		Surface *K=A->processLine("k/x 1.0 1.0 1.0 1.0");
		Cone tK;
		tK.setSurface("k/x 1.0 1.0 1.0 1.0");
		TS_ASSERT_EQUALS(extractString(*K),extractString(tK));
		Surface *T=A->processLine("t/x 1 1 1 2 3 4");
		Torus tT;
		tT.setSurface("t/x 1 1 1 2 3 4");
		TS_ASSERT_EQUALS(extractString(*T),extractString(tT));
	}
private:
	std::string extractString(const Surface& pv)
	{
		//dump output to sting
		std::ostringstream output;
		output.exceptions( std::ios::failbit | std::ios::badbit );
		TS_ASSERT_THROWS_NOTHING(pv.write(output));
		return output.str();
	}
};
#endif