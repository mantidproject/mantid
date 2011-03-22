#ifndef TEST_NORMAL_PARAMETER_H_
#define TEST_NORMAL_PARAMETER_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <boost/scoped_ptr.hpp>
#include "MantidMDAlgorithms/NormalParameter.h"

class NormalParameterTest: public CxxTest::TestSuite
{
public:

  void testCreate()
  {

    Mantid::MDAlgorithms::NormalParameter normal(0, 1, 2);
    TSM_ASSERT_EQUALS("NormalParameter getX() is not wired-up correctly.", 0, normal.getX());
    TSM_ASSERT_EQUALS("NormalParameter getY() is not wired-up correctly.", 1, normal.getY());
    TSM_ASSERT_EQUALS("NormalParameter getZ() is not wired-up correctly.", 2, normal.getZ());
  }

  void testIsValid()
  {
    Mantid::MDAlgorithms::NormalParameter normal(0, 0, 0);
    TSM_ASSERT_EQUALS("The NormalParameter should be valid.", true, normal.isValid());
  }

  void testAssigment()
  {
    using namespace Mantid::MDAlgorithms;
    NormalParameter A(0, 1, 2);
    NormalParameter B;
    A = B;
    TSM_ASSERT_EQUALS("Assigned NormalParameter getX() is not correct.", 0, A.getX());
    TSM_ASSERT_EQUALS("Assigned NormalParameter getY() is not correct.", 0, A.getY());
    TSM_ASSERT_EQUALS("Assigned NormalParameter getZ() is not correct.", 0, A.getZ());
    TSM_ASSERT_EQUALS("Assigned NormalParameter isValid() is not correct.", false, A.isValid());
  }

  void testDefaultInvalid()
  {
    Mantid::MDAlgorithms::NormalParameter normal;
    TSM_ASSERT_EQUALS("Should be invalid!", false, normal.isValid());
  }

  void testIsNotValid()
  {
    Mantid::MDAlgorithms::NormalParameter normal(0, 0, 0);
    TSM_ASSERT_EQUALS("NormalParameter constructed via parameterless constructor should be invalid.",
        true, normal.isValid());
  }

  void testClone()
  {
    Mantid::MDAlgorithms::NormalParameter original(0, 1, 2);
    boost::scoped_ptr<Mantid::MDAlgorithms::NormalParameter> cloned(original.clone());

    TSM_ASSERT_EQUALS("Cloned NormalParameter getX() is not same as original.", 0, cloned->getX());
    TSM_ASSERT_EQUALS("Cloned NormalParameter getY() is not same as original.", 1, cloned->getY());
    TSM_ASSERT_EQUALS("Cloned NormalParameter getZ() is not same as original.", 2, cloned->getZ());
    TSM_ASSERT_EQUALS("Cloned NormalParameter isValid() is not same as original.", original.isValid(),
        cloned->isValid());
  }

  void testCopy()
  {
    Mantid::MDAlgorithms::NormalParameter original(0, 1, 2);
    Mantid::MDAlgorithms::NormalParameter copy(original);
    TSM_ASSERT_EQUALS("Copied NormalParameter getX() is not same as original.", 0, copy.getX());
    TSM_ASSERT_EQUALS("Copied NormalParameter getY() is not same as original.", 1, copy.getY());
    TSM_ASSERT_EQUALS("Copied NormalParameter getZ() is not same as original.", 2, copy.getZ());
    TSM_ASSERT_EQUALS("Copied NormalParameter isValid() is not same as original.", original.isValid(),
        copy.isValid());
  }

  void testGetNameFunctionsEquivalent()
  {
    Mantid::MDAlgorithms::NormalParameter normal(0, 0, 0);
TSM_ASSERT_EQUALS  ("The static name and the dynamic name of the NormalParameter do not match.", normal.getName(), Mantid::MDAlgorithms::NormalParameter::parameterName())
}

void testReflect()
{
  Mantid::MDAlgorithms::NormalParameter normal(1, 2, 3);
  Mantid::MDAlgorithms::NormalParameter reflected = normal.reflect();

  TSM_ASSERT_EQUALS("Reflected normal x value is not negative of original.", -1, reflected.getX() );
  TSM_ASSERT_EQUALS("Reflected normal y value is not negative of original.", -2, reflected.getY() );
  TSM_ASSERT_EQUALS("Reflected normal z value is not negative of original.", -3, reflected.getZ() );
}

void testEqual()
{
  using namespace Mantid::MDAlgorithms;
  NormalParameter A(1, 2, 3);
  NormalParameter B(1, 2, 3);

  TSM_ASSERT_EQUALS("The two Normal instances are not considered equal, but should be.", A, B);
}

void testNotEqual()
{
  //Test unequal combinations
  using namespace Mantid::MDAlgorithms;
  NormalParameter A(1, 2, 3);
  NormalParameter B(0, 2, 3);
  NormalParameter C(1, 0, 3);
  NormalParameter D(1, 2, 0);
  TSM_ASSERT_DIFFERS("The two Normal instances are considered equal, but should not be.", A, B);
  TSM_ASSERT_DIFFERS("The two Normal instances are considered equal, but should not be.", A, C);
  TSM_ASSERT_DIFFERS("The two Normal instances are considered equal, but should not be.", A, D);
}

void testIsUnitVector()
{
  using namespace Mantid::MDAlgorithms;
  NormalParameter notUnit(1, 2, 3);
  NormalParameter unit(0, 1, 0);
  TSM_ASSERT("This is not a unit vector", !notUnit.isUnitVector());
  TSM_ASSERT("This is a unit vector", unit.isUnitVector());
}


void testAsUnitVector()
{
  using namespace Mantid::MDAlgorithms;
  NormalParameter A(2, 0, 0);
  NormalParameter B = A.asUnitVector();
  TSM_ASSERT("This is not a unit vector", !A.isUnitVector());
  TSM_ASSERT("This is a unit vector", B.isUnitVector());
  TSM_ASSERT_EQUALS("x component incorrect", 1, B.getX());
  TSM_ASSERT_EQUALS("y component incorrect", 0, B.getY());
  TSM_ASSERT_EQUALS("z component incorrect", 0, B.getZ());

}

};

#endif
