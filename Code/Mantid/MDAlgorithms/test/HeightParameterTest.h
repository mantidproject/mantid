#ifndef TEST_HEIGHT_PARAMETER_H_
#define TEST_HEIGHT_PARAMETER_H_

#include <cxxtest/TestSuite.h>
#include <boost/scoped_ptr.hpp>
#include "SingleValueParameterBaseTest.h"
#include "MantidMDAlgorithms/HeightParameter.h"

using namespace Mantid::MDAlgorithms;

class HeightParameterTest :  public CxxTest::TestSuite, public SingleValueParameterTests<HeightParameter>
{
public:

  void testGetName()
  {
    SingleValueParameterTests::testGetName("HeightParameter");
  }

  void testIsValid()
  {
    SingleValueParameterTests::testIsValid();
  }

  void testIsNotValid()
  {
    SingleValueParameterTests::testIsNotValid();
  }

  void testAssigment()
  {
    SingleValueParameterTests::testAssigment();
  }

  void testClone()
  {
    SingleValueParameterTests::testClone();
  }

  void testCopy()
  {
    SingleValueParameterTests::testCopy();
  }

  void testToXML()
  {
    SingleValueParameterTests::testToXML();    
  }

  void testEqual()
  {
    SingleValueParameterTests::testEqual();
  }

  void testNotEqual()
  {
    SingleValueParameterTests::testNotEqual();
  }
};

#endif
