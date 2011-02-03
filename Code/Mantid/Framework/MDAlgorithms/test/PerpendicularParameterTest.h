#ifndef PERPENDICULAR_PARAMETER_TEST_H_
#define PERPENDICULAR_PARAMETER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <boost/scoped_ptr.hpp>
#include "MantidMDAlgorithms/PerpendicularParameter.h"

class PerpendicularParameterTest: public CxxTest::TestSuite
{
public:

  void testAsImplicitFunctionParameter()
  {
    using Mantid::MDAlgorithms::PerpendicularParameter;
    using Mantid::API::ImplicitFunctionParameter;
    //Ensure that CRTP does not interfere with base/derived behaviour.
    PerpendicularParameter perpendicular(0, 1, 2);
    ImplicitFunctionParameter& param = perpendicular;

    //Verify polymoric usage using clone method implementation.
    boost::scoped_ptr<ImplicitFunctionParameter> cloned(param.clone());
    TSM_ASSERT_EQUALS("Clone not working via base type", PerpendicularParameter::parameterName(), cloned->getName())
  }

  void testCreate()
  {

    Mantid::MDAlgorithms::PerpendicularParameter perpendicular(0, 1, 2);
    TSM_ASSERT_EQUALS("PerpendicularParameter getX() is not wired-up correctly.", 0, perpendicular.getX() );
    TSM_ASSERT_EQUALS("PerpendicularParameter getY() is not wired-up correctly.", 1, perpendicular.getY() );
    TSM_ASSERT_EQUALS("PerpendicularParameter getZ() is not wired-up correctly.", 2, perpendicular.getZ() );
  }

  void testIsValid()
  {
    Mantid::MDAlgorithms::PerpendicularParameter perpendicular(0, 0, 0);
    TSM_ASSERT_EQUALS("The PerpendicularParameter should be valid.", true, perpendicular.isValid());
  }


  void testDefaultInvalid()
  {
    Mantid::MDAlgorithms::PerpendicularParameter perpendicular;
    TSM_ASSERT_EQUALS("Should be invalid!", false, perpendicular.isValid() );
  }

  void testAssigment()
  {
    using namespace Mantid::MDAlgorithms;
    PerpendicularParameter A(0, 1, 2);
    PerpendicularParameter B;
    A = B;
    TSM_ASSERT_EQUALS("Assigned PerpendicularParameter getX() is not correct.", 0, A.getX() );
    TSM_ASSERT_EQUALS("Assigned PerpendicularParameter getY() is not correct.", 1, A.getY() );
    TSM_ASSERT_EQUALS("Assigned PerpendicularParameter getZ() is not correct.", 2, A.getZ() );
    TSM_ASSERT_EQUALS("Assigned PerpendicularParameter isValid() is not same as original.", B.isValid(), A.isValid() );
  }

  void testClone()
  {
    Mantid::MDAlgorithms::PerpendicularParameter original(0, 1, 2);
    boost::scoped_ptr<Mantid::MDAlgorithms::PerpendicularParameter> cloned(original.clone());

    TSM_ASSERT_EQUALS("Cloned PerpendicularParameter getX() is not same as original.", 0, cloned->getX() );
    TSM_ASSERT_EQUALS("Cloned PerpendicularParameter getY() is not same as original.", 1, cloned->getY() );
    TSM_ASSERT_EQUALS("Cloned PerpendicularParameter getZ() is not same as original.", 2, cloned->getZ() );
    TSM_ASSERT_EQUALS("Cloned PerpendicularParameter isValid() is not same as original.", original.isValid(), cloned->isValid() );
  }

  void testCopy()
  {
    Mantid::MDAlgorithms::PerpendicularParameter original(0, 1, 2);
    Mantid::MDAlgorithms::PerpendicularParameter copy(original);
    TSM_ASSERT_EQUALS("Copied PerpendicularParameter getX() is not same as original.", 0, copy.getX() );
    TSM_ASSERT_EQUALS("Copied PerpendicularParameter getY() is not same as original.", 1, copy.getY() );
    TSM_ASSERT_EQUALS("Copied PerpendicularParameter getZ() is not same as original.", 2, copy.getZ() );
    TSM_ASSERT_EQUALS("Copied PerpendicularParameter isValid() is not same as original.", original.isValid(), copy.isValid() );
  }

  void testGetNameFunctionsEquivalent()
  {
    Mantid::MDAlgorithms::PerpendicularParameter perpendicular(0, 0, 0);
    TSM_ASSERT_EQUALS("The static name and the dynamic name of the PerpendicularParameter do not match.", perpendicular.getName(),  Mantid::MDAlgorithms::PerpendicularParameter::parameterName());
  }

  void testToXML()
  {
    Mantid::MDAlgorithms::PerpendicularParameter perpendicular(1, 2, 3);
    TSM_ASSERT_EQUALS("The generated xml for the PerpendicularParameter does not match the specification.", "<Parameter><Type>PerpendicularParameter</Type><Value>1.0000, 2.0000, 3.0000</Value></Parameter>", perpendicular.toXMLString());
  }

  void testEqual()
  {
    using namespace Mantid::MDAlgorithms;
    PerpendicularParameter A(1, 2, 3);
    PerpendicularParameter B(1, 2, 3);

    TSM_ASSERT_EQUALS("The two perpendicular instances are not considered equal, but should be.", A, B);
  }

  void testNotEqual()
  {
    //Test unequal combinations
    using namespace Mantid::MDAlgorithms;
    PerpendicularParameter A(1, 2, 3);
    PerpendicularParameter B(0, 2, 3);
    PerpendicularParameter C(1, 0, 3);
    PerpendicularParameter D(1, 2, 0);
    TSM_ASSERT_DIFFERS("The two perpendicular instances are considered equal, but should not be.", A, B);
    TSM_ASSERT_DIFFERS("The two perpendicular instances are considered equal, but should not be.", A, C);
    TSM_ASSERT_DIFFERS("The two perpendicular instances are considered equal, but should not be.", A, D);
  }
};
#endif
