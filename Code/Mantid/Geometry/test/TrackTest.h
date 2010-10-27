#ifndef MANTID_TESTTRACK__
#define MANTID_TESTTRACK__

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/V3D.h"

using namespace Mantid;
using namespace Geometry;
class TrackTest : public CxxTest::TestSuite
{
public:
	void testConstructor(){
		Track A(V3D(0,0,0),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.startPoint(),V3D(0.0,0.0,0));
		TS_ASSERT_EQUALS(A.direction(),V3D(1.0,0.0,0.0));
	}

	void testTrackParamConstructor(){
		Track A(V3D(1,1,1),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.startPoint(),V3D(1.0,1.0,1));
		TS_ASSERT_EQUALS(A.direction(),V3D(1.0,0.0,0.0));
		Track B(A);
		TS_ASSERT_EQUALS(B.startPoint(),V3D(1.0,1.0,1));
		TS_ASSERT_EQUALS(B.direction(),V3D(1.0,0.0,0.0));
	}

	void testIterator(){
		Track A(V3D(1,1,1),V3D(1.0,0.0,0.0));
		Track::LType::const_iterator iterBegin=A.begin();
		Track::LType::const_iterator iterEnd=A.end();
		TS_ASSERT_EQUALS(iterBegin,iterEnd);
	}

	void testAddLink(){
		Track A(V3D(1,1,1),V3D(1.0,0.0,0.0));
		A.addLink(V3D(2,2,2),V3D(3,3,3),2.0,NULL);
		Track::LType::const_iterator iterBegin=A.begin();
		Track::LType::const_iterator iterEnd=A.end();
		iterBegin++;
		TS_ASSERT_EQUALS(iterBegin,iterEnd);
	}

	void testreset(){
		Track A(V3D(1,1,1),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.startPoint(),V3D(1.0,1.0,1));
		TS_ASSERT_EQUALS(A.direction(),V3D(1.0,0.0,0.0));
		A.reset(V3D(2,2,2),V3D(0.0,1.0,0.0));
		TS_ASSERT_EQUALS(A.startPoint(),V3D(2.0,2.0,2));
		TS_ASSERT_EQUALS(A.direction(),V3D(0.0,1.0,0.0));
	}

	void testAssignment(){ //Also have to test the Links and Points
		Track A(V3D(1,1,1),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.startPoint(),V3D(1.0,1.0,1));
		TS_ASSERT_EQUALS(A.direction(),V3D(1.0,0.0,0.0));
		Track B(V3D(2,2,2),V3D(0.0,1.0,0.0));
		TS_ASSERT_EQUALS(B.startPoint(),V3D(2.0,2.0,2));
		TS_ASSERT_EQUALS(B.direction(),V3D(0.0,1.0,0.0));		
		B=A;
		TS_ASSERT_EQUALS(B.startPoint(),V3D(1.0,1.0,1));
		TS_ASSERT_EQUALS(B.direction(),V3D(1.0,0.0,0.0));
	}

	void testBuildLink(){
		Track A(V3D(-5,-5,0),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.startPoint(),V3D(-5.0,-5.0,0.0));
		TS_ASSERT_EQUALS(A.direction(),V3D(1.0,0.0,0.0));
		A.addPoint(1,V3D(-5.0,-2.0,0.0)); //Entry at -5,-2,0
		A.addPoint(-1,V3D(-5.0,2.0,0.0)); //Exit point at -5,2,0
		A.buildLink();
		//Check track length
		int index=0;
		for(Track::LType::const_iterator it=A.begin();it!=A.end();++it){
			TS_ASSERT_DELTA(it->distFromStart,7,0.0001);
			TS_ASSERT_DELTA(it->distInsideObject,4,0.0001);
			TS_ASSERT_EQUALS(it->componentID,(ComponentID)NULL);
			TS_ASSERT_EQUALS(it->entryPoint,V3D(-5.0,-2.0,0.0));
			TS_ASSERT_EQUALS(it->exitPoint,V3D(-5.0,2.0,0.0));
			index++;
		}
		TS_ASSERT_EQUALS(index,1);
	}

	void testRemoveCojoins(){
		Track A(V3D(1,1,1),V3D(1.0,0.0,0.0));
		A.addLink(V3D(2,2,2),V3D(3,3,3),2.0);
		A.addLink(V3D(2.0001,2.0001,2.0001),V3D(3,3,3),2.001);
		//Check track length
		int index=0;
		for(Track::LType::const_iterator it=A.begin();it!=A.end();++it){
			index++;
		}
		TS_ASSERT_EQUALS(index,2);
		A.removeCojoins();
		index=0;
		{
		for(Track::LType::const_iterator it=A.begin();it!=A.end();++it){
			index++;
		}
		}
		TS_ASSERT_EQUALS(index,1);
	}

	void testNonComplete(){
		Track A(V3D(1,1,1),V3D(1.0,0.0,0.0));
		A.addLink(V3D(2,2,2),V3D(3,3,3),2.0);
		A.addLink(V3D(2.0001,2.0001,2.0001),V3D(3,3,3),2.001);
		TS_ASSERT(A.nonComplete()>0);
		Track B(V3D(1,1,1),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(B.startPoint(),V3D(1.0,1.0,1));
		TS_ASSERT_EQUALS(B.direction(),V3D(1.0,0.0,0.0));
		B.addLink(V3D(1,1,1),V3D(1,3,1),0.0);
		B.addLink(V3D(1,3,1),V3D(1,5,1),2.0);
		TS_ASSERT_EQUALS(B.nonComplete(),0);
	}
};

#endif
