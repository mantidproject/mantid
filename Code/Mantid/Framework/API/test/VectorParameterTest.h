#ifndef VECTOR_IMPLICIT_FUNCTION_PARAMETER_TEST_H_
#define VECTOR_IMPLICIT_FUNCTION_PARAMETER_TEST_H_

#include "MantidAPI/VectorParameter.h"
#include <cxxtest/TestSuite.h>

class VectorParameterTest : public CxxTest::TestSuite
{
private:

  //Declare a concrete type with elements of type double for testing.
  DECLARE_VECTOR_PARAMETER(ConcreteVectorDblParam, double)
  //Declare a concrete type with elements of type bool for testing.
  DECLARE_VECTOR_PARAMETER(ConcreteVectorBoolParam, bool)

public:

  void testConstructionInvalid()
  {
    ConcreteVectorDblParam param;
    TSM_ASSERT("Nothing added. Should not be valid.", !param.isValid());
    TS_ASSERT_EQUALS(0, param.getSize());
  }

  void testAddValues()
  {
    ConcreteVectorDblParam param;
    param.addValue(1);
    TSM_ASSERT("Should be valid now that a value has been added.", param.isValid());
    TS_ASSERT_EQUALS(1, param.getSize());
  }

  void testEquality()
  {
    ConcreteVectorDblParam A; //Leave as invalid

    ConcreteVectorDblParam B; //Add some values
    B.addValue(1);
    B.addValue(2);

    ConcreteVectorDblParam C; //Duplicate B
    C.addValue(1);
    C.addValue(2);

    ConcreteVectorDblParam D; //Add some other values
    D.addValue(3);

    TS_ASSERT(A != B);
    TS_ASSERT(B == C);
    TS_ASSERT(B != D);
  }

  void testCopyInvalidObjects()
  {
    ConcreteVectorDblParam original;
    ConcreteVectorDblParam copy(original);

    TS_ASSERT(!copy.isValid());
    TS_ASSERT_EQUALS(0, copy.getSize());
  }

  void testCopyValidObjects()
  {
    ConcreteVectorDblParam original;
    original.addValue(1);
    ConcreteVectorDblParam copy(original);
    TS_ASSERT(original == copy); //NB. This assumes that the equality test above has passed!

  }

  void testAssignement()
  {
    ConcreteVectorDblParam A;
    A.addValue(1);
    A.addValue(2); //Now has size == 2

    ConcreteVectorDblParam B;
    B.addValue(3); 
    B.addValue(4); 
    A = B;
    TS_ASSERT(A == B);
  }

  void testGetName()
  {
    ConcreteVectorDblParam param;
    TS_ASSERT_EQUALS("ConcreteVectorDblParam", param.getName());
  }

  void testToXMLStringThrows()
  {
    ConcreteVectorDblParam param;
    TSM_ASSERT_THROWS("Should throw if trying to serialize and invalid object", param.toXMLString(), std::runtime_error);
  }

  void testToXMLString()
  {
    ConcreteVectorDblParam param;
    param.addValue(1);
    TS_ASSERT_EQUALS("<Parameter><Type>ConcreteVectorDblParam</Type><Value>1.0000</Value></Parameter>", param.toXMLString());
    param.addValue(2);
    param.addValue(3);
    TS_ASSERT_EQUALS("<Parameter><Type>ConcreteVectorDblParam</Type><Value>1.0000,2.0000,3.0000</Value></Parameter>", param.toXMLString());
  }

  void testAsArray()
  {
    ConcreteVectorDblParam param;
    param.addValue(1);
    param.addValue(2);
    param.addValue(3);
    TS_ASSERT_EQUALS(1, param[0]);
    TS_ASSERT_EQUALS(2, param[1]);
    TS_ASSERT_EQUALS(3, param[2]);
  }

  void testClone()
  {
    ConcreteVectorDblParam param;
    param.addValue(1);
    param.addValue(2);
    param.addValue(3);

    ConcreteVectorDblParam* clone = param.clone();
    TS_ASSERT(*clone == param);
    delete clone;
  }

  void testAddBoolValues()
  {
    ConcreteVectorBoolParam param;
    param.addValue(true);
    param.addValue(false);
    param.addValue(true);

    TS_ASSERT_EQUALS("<Parameter><Type>ConcreteVectorBoolParam</Type><Value>1,0,1</Value></Parameter>", param.toXMLString());
  }

};

#endif