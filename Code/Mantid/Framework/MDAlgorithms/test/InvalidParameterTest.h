#ifndef TEST_INVALID_PARAMETER_H_
#define TEST_INVALID_PARAMETER_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <boost/scoped_ptr.hpp>
#include "MantidMDAlgorithms/InvalidParameter.h"

class InvalidParameterTest : public CxxTest::TestSuite
{
public:

    void testIsValid()
    {
        using namespace Mantid::MDAlgorithms;
        InvalidParameter invalidParam;
        TSM_ASSERT_EQUALS("The InvalidParameter should always be invalid", false, invalidParam.isValid());
    }

    void testClone()
    {
        using namespace Mantid::MDAlgorithms;
        InvalidParameter original("1");
        boost::scoped_ptr<InvalidParameter> cloned(original.clone());
        TSM_ASSERT_EQUALS("The parameter value has not been cloned.", "1", cloned->getValue());
    }

    void testCopy()
    {
        using namespace Mantid::MDAlgorithms;
        InvalidParameter original("1");
        InvalidParameter copy(original);
        TSM_ASSERT_EQUALS("The parameter value has not been copied.", "1", copy.getValue());
    }

    void testGetNameFunctionsEquivalent()
    {
        using namespace Mantid::MDAlgorithms;
        InvalidParameter origin;
        TSM_ASSERT_EQUALS("The static name and the dynamic name of the InvalidParameter do not match.", origin.getName(),  InvalidParameter::parameterName())
    }

    void testToXMLThrows()
    {
        using namespace Mantid::MDAlgorithms;
        InvalidParameter invalidParam;
        TSM_ASSERT_THROWS("Should have thrown runtime_error exception as it is not possible to represent and invalid parameter in xml.",  invalidParam.toXMLString(), std::runtime_error );
    }

};

#endif
