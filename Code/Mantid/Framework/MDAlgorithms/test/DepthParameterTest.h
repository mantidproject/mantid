#ifndef TEST_DEPTH_PARAMETER_H_
#define TEST_DEPTH_PARAMETER_H_

#include <cxxtest/TestSuite.h>
#include <boost/scoped_ptr.hpp>
#include "SingleValueParameterBaseTest.h"
#include "MantidMDAlgorithms/DepthParameter.h"

using namespace Mantid::MDAlgorithms;

typedef SingleValueParameterTests<DepthParameter> SVPTDepth;
class DepthParameterTest :  public CxxTest::TestSuite, public SVPTDepth
{
public:

  void testGetName()
  {
    SVPTDepth::testGetName("DepthParameter");
  }

  void testIsValid()
  {
    SVPTDepth::testIsValid();
  }

  void testIsNotValid()
  {
    SVPTDepth::testIsNotValid();
  }

  void testAssigment()
  {
    SVPTDepth::testAssigment();
  }

  void testClone()
  {
    SVPTDepth::testClone();
  }

  void testCopy()
  {
    SVPTDepth::testCopy();
  }

  void testToXML()
  {
    SVPTDepth::testToXML();
  }

  void testEqual()
  {
    SVPTDepth::testEqual();
  }

  void testNotEqual()
  {
    SVPTDepth::testNotEqual();
  }

  void testInvalidIfUnsigned()
  {
    SVPTDepth::testInvalidIfUnsigned();
  }
};

#endif
