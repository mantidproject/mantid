// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VECTOR_IMPLICIT_FUNCTION_PARAMETER_TEST_H_
#define VECTOR_IMPLICIT_FUNCTION_PARAMETER_TEST_H_

#include "MantidAPI/VectorParameter.h"
#include <cxxtest/TestSuite.h>

class VectorParameterTest : public CxxTest::TestSuite {
private:
  // Declare a concrete type with elements of type double for testing.
  DECLARE_VECTOR_PARAMETER(ConcreteVectorDblParam, double)
  // Declare a concrete type with elements of type bool for testing.
  DECLARE_VECTOR_PARAMETER(ConcreteVectorBoolParam, bool)

public:
  void testConstructionInvalid() {
    ConcreteVectorDblParam param;
    TSM_ASSERT("Nothing added. Should not be valid.", !param.isValid());
    TS_ASSERT_EQUALS(0, param.getSize());
  }

  void testAddValues() {
    ConcreteVectorDblParam param(1);
    param.addValue(0, 1);
    TSM_ASSERT("Should be valid now that a value has been added.",
               param.isValid());
    TS_ASSERT_EQUALS(1, param.getSize());
  }

  void testEquality() {
    ConcreteVectorDblParam A; // Leave as invalid

    ConcreteVectorDblParam B(2); // Add some values
    B.addValue(0, 1);
    B.addValue(1, 2);

    ConcreteVectorDblParam C(2); // Duplicate B
    C.addValue(0, 1);
    C.addValue(1, 2);

    ConcreteVectorDblParam D(1); // Add some other values
    D.addValue(0, 3);

    TS_ASSERT(A != B);
    TS_ASSERT(B == C);
    TS_ASSERT(B != D);
  }

  void testCopyInvalidObjects() {
    ConcreteVectorDblParam original;
    ConcreteVectorDblParam copy(original);

    TS_ASSERT(!copy.isValid());
    TS_ASSERT_EQUALS(0, copy.getSize());
  }

  void testCopyValidObjects() {
    ConcreteVectorDblParam original(1);
    original.addValue(0, 1);
    ConcreteVectorDblParam copy(original);
    TS_ASSERT(
        original ==
        copy); // NB. This assumes that the equality test above has passed!
  }

  void testAssignement() {
    ConcreteVectorDblParam A(2);
    A.addValue(0, 1);
    A.addValue(1, 2); // Now has size == 2

    ConcreteVectorDblParam B(2);
    B.addValue(0, 3);
    B.addValue(1, 4);
    A.assignFrom(B);
    TS_ASSERT(A == B);
    TS_ASSERT(A.getPointerToStart() != B.getPointerToStart());
  }

  void testGetName() {
    ConcreteVectorDblParam param;
    TS_ASSERT_EQUALS("ConcreteVectorDblParam", param.getName());
  }

  void testToXMLStringThrows() {
    ConcreteVectorDblParam param;
    TSM_ASSERT_THROWS("Should throw if trying to serialize and invalid object",
                      param.toXMLString(), const std::runtime_error &);
  }

  void testToXMLString() {
    ConcreteVectorDblParam param(3);
    param.addValue(0, 1);
    param.addValue(1, 2);
    param.addValue(2, 3);
    TS_ASSERT_EQUALS("<Parameter><Type>ConcreteVectorDblParam</"
                     "Type><Value>1.0000,2.0000,3.0000</Value></Parameter>",
                     param.toXMLString());
  }

  void testAsArray() {
    ConcreteVectorDblParam param(3);
    param.addValue(0, 1);
    param.addValue(1, 2);
    param.addValue(2, 3);
    TS_ASSERT_EQUALS(1, param[0]);
    TS_ASSERT_EQUALS(2, param[1]);
    TS_ASSERT_EQUALS(3, param[2]);
  }

  void testClone() {
    ConcreteVectorDblParam param(3);
    param.addValue(0, 1);
    param.addValue(1, 2);
    param.addValue(2, 3);

    ConcreteVectorDblParam *clone = param.clone();
    TS_ASSERT(*clone == param);
    delete clone;
  }

  void testAddBoolValues() {
    ConcreteVectorBoolParam param(3);
    param.addValue(0, true);
    param.addValue(1, false);
    param.addValue(2, true);

    TS_ASSERT_EQUALS("<Parameter><Type>ConcreteVectorBoolParam</"
                     "Type><Value>1,0,1</Value></Parameter>",
                     param.toXMLString());
  }
};

#endif
