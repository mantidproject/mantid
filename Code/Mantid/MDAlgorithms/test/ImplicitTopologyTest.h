#ifndef TEST_IMPLICIT_TOPOLOGY_H_
#define TEST_IMPLICIT_TOPOLOGY_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>
#include <vector>
#include <memory>
#include "ImplicitTopology.h"
#include "MantidAPI/Point3D.h"


class ImplicitTopologyTest : public CxxTest::TestSuite
{
    private:
	
	class MockPoint3D: public Mantid::API::Point3D
    {
    public:
	    MOCK_CONST_METHOD0(getX, double());
        MOCK_CONST_METHOD0(getY, double());
        MOCK_CONST_METHOD0(getZ, double());
    };
	
public:

    void testOrder()
    {
		Mantid::API::Point3D* pointA = new MockPoint3D;
		Mantid::API::Point3D* pointB = new MockPoint3D;
		Mantid::API::Point3D* pointC = new MockPoint3D;
		
		Mantid::API::Point3D** pointArry = new Mantid::API::Point3D*[3];
		pointArry[0] = pointA;
		pointArry[1] = pointB;
		pointArry[2] = pointC;
		
		Mantid::MDAlgorithms::ImplicitTopology topology;
		topology.applyOrdering(pointArry);
		
		TSM_ASSERT_EQUALS("The elements have been sorted, this should not have occured for this type of topology", pointA, pointArry[0]);
		TSM_ASSERT_EQUALS("The elements have been sorted, this should not have occured for this type of topology", pointB, pointArry[1]);
		TSM_ASSERT_EQUALS("The elements have been sorted, this should not have occured for this type of topology", pointC, pointArry[2]);
		
		delete[] pointArry;
    }
};


#endif