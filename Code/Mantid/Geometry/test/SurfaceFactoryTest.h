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
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidGeometry/Surfaces/Cone.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/SurfaceFactory.h"


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
		delete P;
		delete B;
		delete C;
		delete K;
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
		delete P;
		delete B;
		delete C;
		delete K;
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
		delete P;
		delete S;
		delete C;
		delete K;
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
