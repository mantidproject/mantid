#ifndef MD_INDEX_CALCULATOR_TEST_H_
#define MD_INDEX_CALCULATOR_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDIndexCalculator.h"

class MDIndexCalculatorTest : public CxxTest::TestSuite
{

public:

  void testCalculatorSubdivideIndex()
  {
    Mantid::MDDataObjects::MDWorkspaceIndexCalculator<3> calculator;
    calculator.setDimensionSize(0,3);
    calculator.setDimensionSize(1,5);
    calculator.setDimensionSize(2,4);
    calculator.checkValidSetUp();

    std::vector<size_t> result = calculator.calculateDimensionIndexes(4);
    TS_ASSERT_EQUALS(1, result[0]);
    TS_ASSERT_EQUALS(1, result[1]);
    TS_ASSERT_EQUALS(0, result[2]);
  }

  void testCalculatorGenerateSingleDimensionIndex()
  {
    std::vector<size_t> indexes;
    indexes.push_back(2); // In x
    indexes.push_back(4); // In y
    indexes.push_back(3); // In z

    Mantid::MDDataObjects::MDWorkspaceIndexCalculator<3> calculator;
    calculator.setDimensionSize(0,3);
    calculator.setDimensionSize(1,5);
    calculator.setDimensionSize(2,4);
    calculator.checkValidSetUp();

    size_t result = calculator.calculateSingleDimensionIndex(indexes);
    TS_ASSERT_EQUALS(59, result);
  }

  void testSelfCheck()
  {
    //The two calculation methods are inverse functions of each other.
    Mantid::MDDataObjects::MDWorkspaceIndexCalculator<3> calculator;
    calculator.setDimensionSize(0, 3);
    calculator.setDimensionSize(1, 5);
    calculator.setDimensionSize(2, 4);
    calculator.checkValidSetUp();

    size_t startingSingleDimensionIndex = 5;
    std::vector<size_t> result = calculator.calculateDimensionIndexes(startingSingleDimensionIndex);
    size_t endSingleDimensionIndex = calculator.calculateSingleDimensionIndex(result);

    TSM_ASSERT_EQUALS("inverse relationship is not working.",startingSingleDimensionIndex, endSingleDimensionIndex);
  }
};

#endif
