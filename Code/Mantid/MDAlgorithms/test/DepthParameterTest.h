#ifndef TEST_DEPTH_PARAMETER_H_
#define TEST_DEPTH_PARAMETER_H_

#include <cxxtest/TestSuite.h>
#include <boost/scoped_ptr.hpp>
#include "SingleValueParameterBaseTest.h"
#include "MantidMDAlgorithms/DepthParameter.h"

using namespace Mantid::MDAlgorithms;

class DepthParameterTest :  public CxxTest::TestSuite, public SingleValueParameterTests<DepthParameter>
{
public:

  void testGetName()
  {
    SingleValueParameterTests::testGetName("DepthParameter");
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
