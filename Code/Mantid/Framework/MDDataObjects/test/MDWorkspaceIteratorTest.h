#ifndef MDWORKSPACE_ITERATOR_TEST_H
#define MDWORKSPACE_ITERATOR_TEST_H

#include <cxxtest/TestSuite.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "MDDataObjects/MDIndexCalculator.h"
#include "MDDataObjects/MDWorkspaceIterator.h"

using Mantid::Geometry::IMDDimension_sptr;
using Mantid::MDDataObjects::MDWorkspaceIndexCalculator;
using Mantid::MDDataObjects::MDWorkspaceIterator;
using namespace testing;

class MDWorkspaceIteratorTest :    public CxxTest::TestSuite
{

private:

  /// Mock IMDDimension allows tests to specify exact expected behavior of dependency.
  class MockIMDDimension : public Mantid::Geometry::IMDDimension {
  public:
    MOCK_CONST_METHOD0(getName,
      std::string());
    MOCK_CONST_METHOD0(getUnits,
      std::string());
    MOCK_CONST_METHOD0(getDimensionId,
      std::string());
    MOCK_CONST_METHOD0(getMaximum,
      double());
    MOCK_CONST_METHOD0(getMinimum,
      double());
    MOCK_CONST_METHOD0(getNBins,
      size_t());
    MOCK_CONST_METHOD0(toXMLString,
      std::string());
    MOCK_CONST_METHOD1(getX,
      double(size_t ind));
  };

public:
  typedef std::vector<IMDDimension_sptr> IMDDimension_sptr_vec;

  void testGetCoordinate()
  {
    int dimensionality = 2;
    MockIMDDimension* pDimensionX = new MockIMDDimension();
    MockIMDDimension* pDimensionY = new MockIMDDimension();

    EXPECT_CALL(*pDimensionX, getX(_)).Times(1);
    EXPECT_CALL(*pDimensionY, getX(_)).Times(1);
    IMDDimension_sptr_vec dimensions(dimensionality);
    dimensions[0] = IMDDimension_sptr(pDimensionX);
    dimensions[1] = IMDDimension_sptr(pDimensionY);

    MDWorkspaceIndexCalculator calculator(dimensionality, 10, 10);
    MDWorkspaceIterator iterator(calculator, dimensions);
    iterator.getCoordinate(0); // Of the 1st dimension should get the 2nd coordinate.
    iterator.getCoordinate(1); // Of the 2nd dimension should get the 2nd coordinate.
    TSM_ASSERT("Extracting coordinates has not utilized dimension x as expected.", Mock::VerifyAndClearExpectations(pDimensionX));
    TSM_ASSERT("Extracting coordinates has not utilized dimension y as expected.", Mock::VerifyAndClearExpectations(pDimensionY));
  }

  void testNext()
  {
    int dimensionality = 1;
    MockIMDDimension* pDimensionX = new MockIMDDimension();

    IMDDimension_sptr_vec dimensions(dimensionality);
    dimensions[0] = IMDDimension_sptr(pDimensionX);

    MDWorkspaceIndexCalculator calculator(dimensionality, 3); // 100 cells.
    MDWorkspaceIterator iterator(calculator, dimensions);
    size_t valuePointerA = iterator.getPointer();
    iterator.next();
    size_t valuePointerB = iterator.getPointer();
    iterator.next();
    size_t valuePointerC = iterator.getPointer();

    TS_ASSERT_EQUALS(0, valuePointerA);
    TS_ASSERT_EQUALS(1, valuePointerB);
    TS_ASSERT_EQUALS(2, valuePointerC);
    TS_ASSERT_EQUALS(false, iterator.next());
  }

  void testLooping()
  {
    int dimensionality = 2;
    MockIMDDimension* pDimensionX = new MockIMDDimension();
    MockIMDDimension* pDimensionY = new MockIMDDimension();

    IMDDimension_sptr_vec dimensions(dimensionality);
    dimensions[0] = IMDDimension_sptr(pDimensionX);
    dimensions[1] = IMDDimension_sptr(pDimensionY);

    MDWorkspaceIndexCalculator calculator(dimensionality, 10, 10); // 100 cells.
    MDWorkspaceIterator iterator(calculator, dimensions);
    size_t valuePointer;
    while(iterator.next())
    {
      valuePointer = iterator.getPointer();
    }
    // 0 to 100. expected value is 99
    TSM_ASSERT_EQUALS("Pointer does not have expected value after looping through entire workspace.", 99, valuePointer );
  }

  void testGetDataSize()
  {
    int dimensionality = 3;
    MockIMDDimension* pDimensionX = new MockIMDDimension();
    MockIMDDimension* pDimensionY = new MockIMDDimension();
    MockIMDDimension* pDimensionZ = new MockIMDDimension();

    IMDDimension_sptr_vec dimensions(dimensionality);
    dimensions[0] = IMDDimension_sptr(pDimensionX);
    dimensions[1] = IMDDimension_sptr(pDimensionY);
    dimensions[2] = IMDDimension_sptr(pDimensionZ);

    MDWorkspaceIndexCalculator calculator(dimensionality, 10, 10, 10); // 1000 cells.
    MDWorkspaceIterator iterator(calculator, dimensions);

    TSM_ASSERT_EQUALS("Incorrect data size calculated", 1000, iterator.getDataSize());
  }

}; 

#endif 
