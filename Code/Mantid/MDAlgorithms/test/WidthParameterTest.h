#ifndef TEST_WIDTH_PARAMETER_H_
#define TEST_WIDTH_PARAMETER_H_

#include <cxxtest/TestSuite.h>
#include <boost/scoped_ptr.hpp>
#include "SingleValueParameterBaseTest.h"
#include "MantidMDAlgorithms/WidthParameter.h"

using namespace Mantid::MDAlgorithms;

class WidthParameterTest :  public CxxTest::TestSuite, public SingleValueParameterTests<WidthParameter>
{
public:

  void testGetName()
  {
    SingleValueParameterTests::testGetName("WidthParameter");
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
