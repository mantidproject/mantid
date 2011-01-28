#ifndef MANTID_TESTSURFACE__
#define MANTID_TESTSURFACE__

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
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidGeometry/Surfaces/Cone.h"
#include "MantidGeometry/Surfaces/Cylinder.h"


using namespace Mantid;
using namespace Geometry;

class SurfaceTest: public CxxTest::TestSuite
{

public:


  void testConeDistance()
    /**
      Test the distance of a point from the cone
    */
  {
    std::vector<std::string> ConeStr;
    ConeStr.push_back("kx 0 1");          // cone at origin
    ConeStr.push_back("k/x 0 0 0 1");     // also cone at origin
    Geometry::V3D P(-1,-1.2,0);
    const double results[]={sin(atan(1.2)-M_PI/4)*sqrt(2.44),
      sin(atan(1.2)-M_PI/4)*sqrt(2.44)};

    std::vector<std::string>::const_iterator vc;
    Cone A;
    int cnt(0);
    for(vc=ConeStr.begin();vc!=ConeStr.end();vc++,cnt++)
      {
	TS_ASSERT_EQUALS(A.setSurface(*vc),0);
	TS_ASSERT_EQUALS(extractString(A),"-1  kx 0 1\n");
	
	const double R=A.distance(P);
	TS_ASSERT_DELTA(R,results[cnt],1e-5);
      }
  }

private:

  std::string extractString(Surface& pv)
  {
    //dump output to sting
    std::ostringstream output;
    output.exceptions( std::ios::failbit | std::ios::badbit );
    TS_ASSERT_THROWS_NOTHING(pv.write(output));
    return output.str();
  }

};

#endif
