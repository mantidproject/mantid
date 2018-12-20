#ifndef PROPERTYHISTORYTEST_H_
#define PROPERTYHISTORYTEST_H_

#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/PropertyHistory.h"

#include <boost/lexical_cast.hpp>
#include <cxxtest/TestSuite.h>
#include <sstream>

using namespace Mantid::Kernel;

class PropertyHistoryTest : public CxxTest::TestSuite {
public:
  void testPopulate() {
    std::string correctOutput = "Name: arg1_param, ";
    correctOutput += "Value: 20, ";
    correctOutput += "Default?: Yes, ";
    correctOutput += "Direction: Input\n";

    // Not really much to test
    PropertyHistory AP("arg1_param", "20", "argument", true, Direction::Input);

    // dump output to sting
    std::ostringstream output;
    TS_ASSERT_THROWS_NOTHING(output << AP);
    TS_ASSERT_EQUALS(output.str(), correctOutput);
  }

  void testOutputWithShortenedValue() {
    std::string correctOutput = "Name: arg1_param, ";
    correctOutput += "Value: 1234567 ... 4567890, ";
    correctOutput += "Default?: Yes, ";
    correctOutput += "Direction: Input\n";

    // a long property that should get shortened
    PropertyHistory propHistory("arg1_param", "123456798012345678901234567890",
                                "argument", true, Direction::Input);

    // dump output to sting
    std::ostringstream output;
    TS_ASSERT_THROWS_NOTHING(propHistory.printSelf(output, 0, 20));
    TS_ASSERT_EQUALS(output.str(), correctOutput);
  }

  /**
   * Test the isEmptyDefault method returns true for unset default-value
   * properties
   * with EMPTY_INT, EMPTY_DBL, EMPTY_LONG
   */
  void testIsEmptyDefault_True() {
    PropertyHistory intProp(
        "arg1_param", boost::lexical_cast<std::string>(Mantid::EMPTY_INT()),
        "number", true, Direction::Input);
    PropertyHistory dblProp(
        "arg2_param", boost::lexical_cast<std::string>(Mantid::EMPTY_DBL()),
        "number", true, Direction::Input);
    PropertyHistory longProp(
        "arg3_param", boost::lexical_cast<std::string>(Mantid::EMPTY_LONG()),
        "number", true, Direction::Input);
    TS_ASSERT_EQUALS(intProp.isEmptyDefault(), true);
    TS_ASSERT_EQUALS(dblProp.isEmptyDefault(), true);
    TS_ASSERT_EQUALS(longProp.isEmptyDefault(), true);
  }

  /**
   * Test the isEmptyDefault method returns false for an output parameter
   */
  void testIsEmptyDefault_WrongDirection() {
    PropertyHistory prop("arg",
                         boost::lexical_cast<std::string>(Mantid::EMPTY_INT()),
                         "number", true, Direction::Output);
    TS_ASSERT_EQUALS(prop.isEmptyDefault(), false);
  }

  /**
   * Test the isEmptyDefault method returns false if the value of EMPTY_INT is
   * not the default
   */
  void testIsEmptyDefault_NotDefault() {
    PropertyHistory prop("arg",
                         boost::lexical_cast<std::string>(Mantid::EMPTY_INT()),
                         "number", false, Direction::Input);
    TS_ASSERT_EQUALS(prop.isEmptyDefault(), false);
  }

  /**
   * Test the isEmptyDefault method returns false if the parameter type is not
   * "number"
   */
  void testIsEmptyDefault_WrongType() {
    PropertyHistory prop("arg",
                         boost::lexical_cast<std::string>(Mantid::EMPTY_INT()),
                         "something", true, Direction::Input);
    TS_ASSERT_EQUALS(prop.isEmptyDefault(), false);
  }

  /**
   * Test the isEmptyDefault method returns false if the value is not EMPTY_XXX
   */
  void testIsEmptyDefault_NotEmpty() {
    PropertyHistory prop(
        "arg", boost::lexical_cast<std::string>(Mantid::EMPTY_INT() - 1),
        "number", true, Direction::Input);
    TS_ASSERT_EQUALS(prop.isEmptyDefault(), false);
  }
};

#endif /* PROPERTYHISTORYTEST_H_*/
