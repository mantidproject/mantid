#ifndef _PARAMETER_TEST_H_
#define _PARAMETER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <boost/scoped_ptr.hpp>
#include "MantidMDAlgorithms/UpParameter.h"

class UpParameterTest: public CxxTest::TestSuite
{
public:

  void testAsImplicitFunctionUpParameter()
  {
    using Mantid::MDAlgorithms::UpParameter;
    using Mantid::API::ImplicitFunctionParameter;
    //Ensure that CRTP does not interfere with base/derived behaviour.
    UpParameter up(0, 1, 2);
    ImplicitFunctionParameter& param = up;

    //Verify polymoric usage using clone method implementation.
    boost::scoped_ptr<ImplicitFunctionParameter> cloned(param.clone());
    TSM_ASSERT_EQUALS("Clone not working via base type", UpParameter::parameterName(), cloned->getName())
  }

  void testCreate()
  {

    Mantid::MDAlgorithms::UpParameter up(0, 1, 2);
    TSM_ASSERT_EQUALS("UpParameter getX() is not wired- correctly.", 0, up.getX() );
    TSM_ASSERT_EQUALS("UpParameter getY() is not wired- correctly.", 1, up.getY() );
    TSM_ASSERT_EQUALS("UpParameter getZ() is not wired- correctly.", 2, up.getZ() );
  }

  void testIsValid()
  {
    Mantid::MDAlgorithms::UpParameter up(0, 0, 0);
    TSM_ASSERT_EQUALS("The UpParameter should be valid.", true, up.isValid());
  }


  void testDefaultInvalid()
  {
    Mantid::MDAlgorithms::UpParameter up;
    TSM_ASSERT_EQUALS("Should be invalid!", false, up.isValid() );
  }

  void testAssigment()
  {
    using namespace Mantid::MDAlgorithms;
    UpParameter A(0, 1, 2);
    UpParameter B;
    A = B;
    TSM_ASSERT_EQUALS("Assigned UpParameter getX() is not correct.", 0, A.getX() );
    TSM_ASSERT_EQUALS("Assigned UpParameter getY() is not correct.", 1, A.getY() );
    TSM_ASSERT_EQUALS("Assigned UpParameter getZ() is not correct.", 2, A.getZ() );
    TSM_ASSERT_EQUALS("Assigned UpParameter isValid() is not same as original.", B.isValid(), A.isValid() );
  }

  void testClone()
  {
    Mantid::MDAlgorithms::UpParameter original(0, 1, 2);
    boost::scoped_ptr<Mantid::MDAlgorithms::UpParameter> cloned(original.clone());

    TSM_ASSERT_EQUALS("Cloned UpParameter getX() is not same as original.", 0, cloned->getX() );
    TSM_ASSERT_EQUALS("Cloned UpParameter getY() is not same as original.", 1, cloned->getY() );
    TSM_ASSERT_EQUALS("Cloned UpParameter getZ() is not same as original.", 2, cloned->getZ() );
    TSM_ASSERT_EQUALS("Cloned UpParameter isValid() is not same as original.", original.isValid(), cloned->isValid() );
  }

  void testCopy()
  {
    Mantid::MDAlgorithms::UpParameter original(0, 1, 2);
    Mantid::MDAlgorithms::UpParameter copy(original);
    TSM_ASSERT_EQUALS("Copied UpParameter getX() is not same as original.", 0, copy.getX() );
    TSM_ASSERT_EQUALS("Copied UpParameter getY() is not same as original.", 1, copy.getY() );
    TSM_ASSERT_EQUALS("Copied UpParameter getZ() is not same as original.", 2, copy.getZ() );
    TSM_ASSERT_EQUALS("Copied UpParameter isValid() is not same as original.", original.isValid(), copy.isValid() );
  }

  void testGetNameFunctionsEquivalent()
  {
    Mantid::MDAlgorithms::UpParameter up(0, 0, 0);
    TSM_ASSERT_EQUALS("The static name and the dynamic name of the UpParameter do not match.", up.getName(),  Mantid::MDAlgorithms::UpParameter::parameterName());
  }

  void testToXML()
  {
    Mantid::MDAlgorithms::UpParameter up(1, 2, 3);
    TSM_ASSERT_EQUALS("The generated xml for the UpParameter does not match the specification.", "<Parameter><Type>UpParameter</Type><Value>1.0000, 2.0000, 3.0000</Value></Parameter>", up.toXMLString());
  }

  void testEqual()
  {
    using namespace Mantid::MDAlgorithms;
    UpParameter A(1, 2, 3);
    UpParameter B(1, 2, 3);

    TSM_ASSERT_EQUALS("The two up instances are not considered equal, but should be.", A, B);
  }

  void testNotEqual()
  {
    //Test unequal combinations
    using namespace Mantid::MDAlgorithms;
    UpParameter A(1, 2, 3);
    UpParameter B(0, 2, 3);
    UpParameter C(1, 0, 3);
    UpParameter D(1, 2, 0);
    TSM_ASSERT_DIFFERS("The two up instances are considered equal, but should not be.", A, B);
    TSM_ASSERT_DIFFERS("The two up instances are considered equal, but should not be.", A, C);
    TSM_ASSERT_DIFFERS("The two up instances are considered equal, but should not be.", A, D);
  }
};



#endif

