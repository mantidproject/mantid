#ifndef TEST_DETECTORSRING_H_
#define TEST_DETECTORSRING_H_

#include "MantidGeometry/Instrument/DetectorsRing.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Geometry;
using namespace Mantid;

class DetectorsRingTest : public CxxTest::TestSuite
{
public:
 static DetectorsRingTest *createSuite() { return new DetectorsRingTest(); }
 static void destroySuite(DetectorsRingTest *suite) { delete suite; }
 //*********************************************
 void testConstructorThrowsInNotRing(){
	 boost::shared_ptr<Mantid::Geometry::DetectorGroup> psGroup = ComponentCreationHelper::createDetectorGroupWith5CylindricalDetectors();
	 TSM_ASSERT_THROWS("should throw on constructing detectors, arranged in line",DetectorsRing *pRing = new DetectorsRing(psGroup->getDetectors()),std::invalid_argument);
 }

private:
	std::auto_ptr<DetectorsRing> pDetRing;
	DetectorsRingTest(){
		pDetRing = std::auto_ptr<DetectorsRing>(new DetectorsRing(detectorsInRing(),false));
	}
	std::vector<boost::shared_ptr<IDetector> > detectorsInRing(){

	 std::vector<boost::shared_ptr<IDetector> > groupMembers;
     // One object
	 double R0=0.5;
	 double h =1.5;
     Object_sptr detShape = ComponentCreationHelper::createCappedCylinder(R0, h, V3D(0.0,0.0,0.0), V3D(0.,1.0,0.), "tube"); 

	 int NY=10;
	 int NX=30;
	 double y_bl = NY*h;
	 double x_bl = NX*R0;

     double Rmin(2.5), Rmax(3.5);
	 double Rmin2(Rmin*Rmin),Rmax2(Rmax*Rmax);
	 double xav(0),yav(0);
	int ic(0);
	for(int j=0;j<NY;j++){
		double y=-0.5*y_bl+j*h;
		for(int i=0;i<NX;i++){
			double x = -0.5*x_bl+i*R0;
			double Rsq = x*x+y*y;
			if(Rsq>=Rmin2 && Rsq<Rmax2){
			     std::ostringstream os;
		         os << "d" << ic;
				 boost::shared_ptr<Detector> det(new Detector(os.str(), ic+1, detShape, NULL));
				 det->setPos(x, y, 2.0);
	    		 groupMembers.push_back(det);
				 // sanity check
				 xav+=x;
				 yav+=y;
			}

          ic++;
		}
	}
	return groupMembers;
	}
};

#endif