#ifndef TEST_HEIGHT_PARAMETER_H_
#define TEST_HEIGHT_PARAMETER_H_

#include <cxxtest/TestSuite.h>
#include <boost/scoped_ptr.hpp>
#include "SingleValueParameterBaseTest.h"
#include "MantidMDAlgorithms/HeightParameter.h"

using namespace Mantid::MDAlgorithms;

typedef SingleValueParameterTests<HeightParameter> SVPTHeight;
class HeightParameterTest :  public CxxTest::TestSuite, public SVPTHeight
{
public:

  void testGetName()
  {
    SVPTHeight::testGetName("HeightParameter");
  }

  void testIsValid()
  {
    SVPTHeight::testIsValid();
  }

  void testIsNotValid()
  {
    SVPTHeight::testIsNotValid();
  }

  void testAssigment()
  {
    SVPTHeight::testAssigment();
  }

  void testClone()
  {
    SVPTHeight::testClone();
  }

  void testCopy()
  {
    SVPTHeight::testCopy();
  }

  void testToXML()
  {
    SVPTHeight::testToXML();
  }

  void testEqual()
  {
    SVPTHeight::testEqual();
  }

  void testNotEqual()
  {
    SVPTHeight::testNotEqual();
  }

  void testInvalidIfUnsigned()
  {
    SVPTHeight::testInvalidIfUnsigned();
  }
};

#endif
