#ifndef TEST_ORIGIN_PARAMETER_H_
#define TEST_ORIGIN_PARAMETER_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <boost/scoped_ptr.hpp>
#include "MantidMDAlgorithms/OriginParameter.h"

class OriginParameterTest : public CxxTest::TestSuite
{
public:

    void testCreate()
    {

        Mantid::MDAlgorithms::OriginParameter origin(0, 1, 2);
        TSM_ASSERT_EQUALS("OriginParameter getX() is not wired-up correctly.", 0, origin.getX() );
        TSM_ASSERT_EQUALS("OriginParameter getY() is not wired-up correctly.", 1, origin.getY() );
        TSM_ASSERT_EQUALS("OriginParameter getZ() is not wired-up correctly.", 2, origin.getZ() );
    }

    void testIsValid()
    {
        Mantid::MDAlgorithms::OriginParameter origin(0, 0, 0);
        TSM_ASSERT_EQUALS("The OriginParameter should be valid.", true, origin.isValid());
    }
	
    void testIsNotValid()
	{
        Mantid::MDAlgorithms::OriginParameter origin(0, 0, 0);
        TSM_ASSERT_EQUALS("OriginParameter constructed via parameterless constructor should be invalid.", true, origin.isValid());	 
	}
	
	
	void testDefaultInvalid()
	{
	  Mantid::MDAlgorithms::OriginParameter origin;
	  TSM_ASSERT_EQUALS("Should be invalid!", false, origin.isValid() );
	}
	
	void testAssigment()
	{
	    using namespace Mantid::MDAlgorithms;
        OriginParameter A(0, 1, 2);
	    OriginParameter B;
		A = B;
        TSM_ASSERT_EQUALS("Assigned OriginParameter getX() is not correct.", 0, A.getX() );
        TSM_ASSERT_EQUALS("Assigned OriginParameter getY() is not correct.", 0, A.getY() );
        TSM_ASSERT_EQUALS("Assigned OriginParameter getZ() is not correct.", 0, A.getZ() );	
		TSM_ASSERT_EQUALS("Assigned OriginParameter isValid() is not same as original.", B.isValid(), A.isValid() );
	}

    void testClone()
    {
        Mantid::MDAlgorithms::OriginParameter original(0, 1, 2);
        boost::scoped_ptr<Mantid::MDAlgorithms::OriginParameter> cloned(original.clone());

        TSM_ASSERT_EQUALS("Cloned OriginParameter getX() is not same as original.", 0, cloned->getX() );
        TSM_ASSERT_EQUALS("Cloned OriginParameter getY() is not same as original.", 1, cloned->getY() );
        TSM_ASSERT_EQUALS("Cloned OriginParameter getZ() is not same as original.", 2, cloned->getZ() );
		TSM_ASSERT_EQUALS("Cloned OriginParameter isValid() is not same as original.", original.isValid(), cloned->isValid() );
    }

    void testCopy()
    {
        Mantid::MDAlgorithms::OriginParameter original(0, 1, 2);
        Mantid::MDAlgorithms::OriginParameter copy(original);
        TSM_ASSERT_EQUALS("Copied OriginParameter getX() is not same as original.", 0, copy.getX() );
        TSM_ASSERT_EQUALS("Copied OriginParameter getY() is not same as original.", 1, copy.getY() );
        TSM_ASSERT_EQUALS("Copied OriginParameter getZ() is not same as original.", 2, copy.getZ() );
		TSM_ASSERT_EQUALS("Copied OriginParameter isValid() is not same as original.", original.isValid(), copy.isValid() );
    }

    void testGetNameFunctionsEquivalent()
    {
        Mantid::MDAlgorithms::OriginParameter origin(0, 0, 0);
        TSM_ASSERT_EQUALS("The static name and the dynamic name of the OriginParameter do not match.", origin.getName(),  Mantid::MDAlgorithms::OriginParameter::parameterName());
    }

    void testToXML()
    {
        Mantid::MDAlgorithms::OriginParameter origin(1, 2, 3);
        TSM_ASSERT_EQUALS("The generated xml for the OriginParameter does not match the specification.", "<Parameter><Type>OriginParameter</Type><Value>1.0000, 2.0000, 3.0000</Value></Parameter>", origin.toXMLString());
    }
	
	void testEqual()
	{
	    using namespace Mantid::MDAlgorithms;
	    OriginParameter A(1, 2, 3);
		OriginParameter B(1, 2, 3);
		
		TSM_ASSERT_EQUALS("The two origin instances are not considered equal, but should be.", A, B);
	}
	
	void testNotEqual()
	{
	    //Test unequal combinations
	    using namespace Mantid::MDAlgorithms;
	    OriginParameter A(1, 2, 3);
		OriginParameter B(0, 2, 3);
		OriginParameter C(1, 0, 3);
		OriginParameter D(1, 2, 0);		
		TSM_ASSERT_DIFFERS("The two origin instances are considered equal, but should not be.", A, B);
		TSM_ASSERT_DIFFERS("The two origin instances are considered equal, but should not be.", A, C);
		TSM_ASSERT_DIFFERS("The two origin instances are considered equal, but should not be.", A, D);
	}

};

#endif
