#ifndef H_TEST_POINT3D
#define H_TEST_POINT3D


#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDImage.h"


using namespace Mantid;
using namespace API;
using namespace MDDataObjects;

class testPoint3D :    public CxxTest::TestSuite
{

public:

    void testPoint3DGetPosistion(void)
	{
			MDDataObjects::point3D point(0,1,2);
			TSM_ASSERT_EQUALS("GetX getter not wired-up correctly.", 0, point.GetX());
			TSM_ASSERT_EQUALS("GetY getter not wired-up correctly.", 1, point.GetY());
			TSM_ASSERT_EQUALS("GetZ getter not wired-up correctly.", 2, point.GetZ());
    }
};

#endif
