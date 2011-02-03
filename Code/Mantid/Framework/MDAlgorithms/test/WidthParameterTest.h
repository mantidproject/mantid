#ifndef TEST_WIDTH_PARAMETER_H_
#define TEST_WIDTH_PARAMETER_H_

#include <cxxtest/TestSuite.h>
#include <boost/scoped_ptr.hpp>
#include "SingleValueParameterBaseTest.h"
#include "MantidMDAlgorithms/WidthParameter.h"

using namespace Mantid::MDAlgorithms;
typedef SingleValueParameterTests<WidthParameter> SVPTWidth;
class WidthParameterTest :  public CxxTest::TestSuite, public SVPTWidth
{
public:

  void testGetName()
  {
    SVPTWidth::testGetName("WidthParameter");
  }

  void testIsValid()
  {
    SVPTWidth::testIsValid();
  }

  void testIsNotValid()
  {
    SVPTWidth::testIsNotValid();
  }

  void testAssigment()
  {
    SVPTWidth::testAssigment();
  }

  void testClone()
  {
    SVPTWidth::testClone();
  }

  void testCopy()
  {
    SVPTWidth::testCopy();
  }

  void testToXML()
  {
    SVPTWidth::testToXML();
  }

  void testEqual()
  {
    SVPTWidth::testEqual();
  }

  void testNotEqual()
  {
    SVPTWidth::testNotEqual();
  }

  void testInvalidIfUnsigned()
  {
    SVPTWidth::testInvalidIfUnsigned();
  }
};

#endif
