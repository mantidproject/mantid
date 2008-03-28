#ifndef MANTID_TESTSURFACE__
#define MANTID_TESTSURFACE__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <ostream>
#include <vector>
#include <algorithm>

#include "../inc/support.h"
#include "../inc/regexSupport.h"
#include "../inc/Matrix.h"
#include "../inc/Vec3D.h"
#include "../inc/Line.h"
#include "../inc/BaseVisit.h"
#include "../inc/Surface.h"
#include "../inc/Cone.h"
#include "../inc/Cylinder.h"
#include "../inc/General.h"
#include "../inc/Sphere.h"
#include "../inc/Torus.h"

using namespace Mantid;
using namespace Geometry;

class testSurface: public CxxTest::TestSuite
{

public:


void testConeDistance()
  /*!
    Test the distance of a point from the cone
  */
{
  std::vector<std::string> ConeStr;
  ConeStr.push_back("kx 0 1");          // cone at origin
  ConeStr.push_back("k/x 0 0 0 1");     // also cone at origin
  Geometry::Vec3D P(-1,-1.2,0);
  const double results[]={sin(atan(1.2)-M_PI/4)*sqrt(2.44),
			  sin(atan(1.2)-M_PI/4)*sqrt(2.44)};

  std::vector<std::string>::const_iterator vc;
  Cone A;
  for(vc=ConeStr.begin();vc!=ConeStr.end();vc++)
    {
      const int retVal=A.setSurface(*vc);
      TS_ASSERT(retVal==0);
      std::cout<<"Cone == ";
      A.write(std::cout);
      const double R=A.distanceTrue(P)<<std::endl;;
      std::cout<<"Distance == "<<R<<" ["<<results[cnt]<<"]"<<std::endl;
      TS_ASSERT_DELTA(R,results[cnt],1e-5);
    }
  return;
}
  
int
testCylinderDistance()
  /*!
    Test the distance of a point from the cone
  */
{
  std::vector<std::string> CylStr;
  CylStr.push_back("cx 1");                // Cylinder origin
  CylStr.push_back("c/x 0.5 0.5 1.0");     // also cylinder at origin
  Geometry::Vec3D P(-1.2,0,0);
  double results[]={ 1-sqrt(0.6*0.6+0.4*0.4),  1.0-sqrt(2*0.1*0.1) };

  std::vector<std::string>::const_iterator vc;
  Cylinder A;
  int cnt(0);
  for(vc=CylStr.begin();vc!=CylStr.end();vc++,cnt++)
    {
      const int retVal=A.setSurface(*vc);
      TS_ASSERT(retVal==0);
      std::cout<<"Cylinder == ";
      A.write(std::cout);
      std::cout<<"TestPoint == "<<P<<std::endl;
      std::cout<<"Distance == "<<A.distanceTrue(P)<<std::endl;;
      std::cout<<"--------------"<<std::endl;
      TS_ASSERT_DELTA(A,distanceTrue(P),results[cnt],0.0001);
    }

  return 0;
}

int
testSurface::testSphereDistance()
  /*!
    Test the distance of a point from the cone
    \retval -1 :: failed build a cone
    \retval -2 :: Failed on the distance calculation
    \retval 0 :: All passed
  */
{
  std::vector<std::string> SphStr;
  SphStr.push_back("so 1");                // sphere origin
  SphStr.push_back("s 1.5 -2.5 1.8 1");     // sphere 
  Geometry::Vec3D P(3,7,4);
  Geometry::Vec3D Q(0,0,4);
  std::vector<std::string>::const_iterator vc;
  Sphere A;

  for(vc=SphStr.begin();vc!=SphStr.end();vc++)
    {
      const int retVal=A.setSurface(*vc);
      TS_ASSERT(retVal==0);
      // Surface distance and Sphere distance are the same:
      if (fabs(A.distanceTrue(P)-A.distance(P))>1e-6)
        {
	  std::cout<<"Sphere == ";
	  A.Surface::write(std::cout);
	  std::cout<<"TestPoint == "<<P<<std::endl;
	  std::cout<<"Distance == "<<A.distanceTrue(P)
		   <<" === "<<A.distance(P)<<std::endl;
	  std::cout<<"--------------"<<std::endl;
	  std::cout<<"Distance == "<<A.distanceTrue(Q)
		   <<" === "<<A.distance(Q)<<std::endl;
	  std::cout<<"--------------"<<std::endl;
	  TS_ASSERT_DELTA(A.distanceTrue(P),A.distance(P),1e-6);
	}
    }
  return 0;
}

};

#endif
