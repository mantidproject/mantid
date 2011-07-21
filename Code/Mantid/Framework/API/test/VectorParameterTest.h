#ifndef VECTOR_IMPLICIT_FUNCTION_PARAMETER_TEST_H_
#define VECTOR_IMPLICIT_FUNCTION_PARAMETER_TEST_H_

#include "MantidAPI/VectorParameter.h"
#include <cxxtest/TestSuite.h>

class VectorParameterTest : public CxxTest::TestSuite
{
private:
  //Declare a concrete type for testing.
  DECLARE_VECTOR_PARAMETER(ConcreteVectorParameter, double)

public:

  void testConstructionInvalid()
  {
    ConcreteVectorParameter param;
    TSM_ASSERT("Nothing added. Should not be valid.", !param.isValid());
    TS_ASSERT_EQUALS(0, param.getSize());
  }

  void testAddValues()
  {
    ConcreteVectorParameter param;
    param.addValue(1);
    TSM_ASSERT("Should be valid now that a value has been added.", param.isValid());
    TS_ASSERT_EQUALS(1, param.getSize());
  }

  void testEquality()
  {
    ConcreteVectorParameter A; //Leave as invalid

    ConcreteVectorParameter B; //Add some values
    B.addValue(1);
    B.addValue(2);

    ConcreteVectorParameter C; //Duplicate B
    C.addValue(1);
    C.addValue(2);

    ConcreteVectorParameter D; //Add some other values
    D.addValue(3);

    TS_ASSERT(A != B);
    TS_ASSERT(B == C);
    TS_ASSERT(B != D);
  }

  void testCopyInvalidObjects()
  {
    ConcreteVectorParameter original;
    ConcreteVectorParameter copy(original);

    TS_ASSERT(!copy.isValid());
    TS_ASSERT_EQUALS(0, copy.getSize());
  }

  void testCopyValidObjects()
  {
    ConcreteVectorParameter original;
    original.addValue(1);
    ConcreteVectorParameter copy(original);
    TS_ASSERT(original == copy); //NB. This assumes that the equality test above has passed!

  }

  void testAssignementThrows()
  {
    ConcreteVectorParameter A;
    A.addValue(1);
    A.addValue(2); //Now has size == 2

    ConcreteVectorParameter B;
    B.addValue(1); //Now has size == 1

    TSM_ASSERT_THROWS("Assignment when sizes are not equal should throw.", A = B, std::runtime_error);
  }

  void testAssignement()
  {
    ConcreteVectorParameter A;
    A.addValue(1);
    A.addValue(2); //Now has size == 2

    ConcreteVectorParameter B;
    B.addValue(3); 
    B.addValue(4); 
    A = B;
    TS_ASSERT(A == B);
  }

  void testGetName()
  {
    ConcreteVectorParameter param;
    TS_ASSERT_EQUALS("ConcreteVectorParameter", param.getName());
  }

  void testToXMLStringThrows()
  {
    ConcreteVectorParameter param;
    TSM_ASSERT_THROWS("Should throw if trying to serialize and invalid object", param.toXMLString(), std::runtime_error);
  }

  void testToXMLString()
  {
    ConcreteVectorParameter param;
    param.addValue(1);
    TS_ASSERT_EQUALS("<Parameter><Type>ConcreteVectorParameter</Type><Value>1.0000</Value></Parameter>", param.toXMLString());
    param.addValue(2);
    param.addValue(3);
    TS_ASSERT_EQUALS("<Parameter><Type>ConcreteVectorParameter</Type><Value>1.0000,2.0000,3.0000</Value></Parameter>", param.toXMLString());
  }

  void testAsArray()
  {
    ConcreteVectorParameter param;
    param.addValue(1);
    param.addValue(2);
    param.addValue(3);
    TS_ASSERT_EQUALS(1, param[0]);
    TS_ASSERT_EQUALS(2, param[1]);
    TS_ASSERT_EQUALS(3, param[2]);
  }

  void testClone()
  {
    ConcreteVectorParameter param;
    param.addValue(1);
    param.addValue(2);
    param.addValue(3);

    ConcreteVectorParameter* clone = param.clone();
    TS_ASSERT(*clone == param);
    delete clone;
  }

};

#endif