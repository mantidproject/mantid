#ifndef MANTID_CUSTOMINTERFACES_USERINPUTVALIDATORTEST_H
#define MANTID_CUSTOMINTERFACES_USERINPUTVALIDATORTEST_H

#include <cxxtest/TestSuite.h>
#include <string>

#include "MantidQtCustomInterfaces/UserInputValidator.h"

using namespace MantidQt::CustomInterfaces;

class UserInputValidatorTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static UserInputValidatorTest *createSuite() { return new UserInputValidatorTest(); }
  static void destroySuite( UserInputValidatorTest *suite ) { delete suite; }

  UserInputValidatorTest()
  {
  }

  void test_validRebin()
  {
    UserInputValidator uiv;
    TS_ASSERT(uiv.checkBins(0.6, 0.1, 1.8));
    TS_ASSERT(uiv.isAllInputValid());
  }

  void test_negativeWidthRebin()
  {
    UserInputValidator uiv;
    TS_ASSERT(!uiv.checkBins(0.6, -0.1, 1.8));
    TS_ASSERT(!uiv.isAllInputValid());
    TS_ASSERT_EQUALS(uiv.generateErrorMessage(), "Please correct the following:\n\nBin width must be a positive value.");
  }

  void test_zeroWidthRebin()
  {
    UserInputValidator uiv;
    TS_ASSERT(!uiv.checkBins(0.6, 0.0, 1.8));
    TS_ASSERT(!uiv.isAllInputValid());
    TS_ASSERT_EQUALS(uiv.generateErrorMessage(), "Please correct the following:\n\nBin width must be non-zero.");
  }

  void test_zeroRangeRebin()
  {
    UserInputValidator uiv;
    TS_ASSERT(!uiv.checkBins(0.6, 0.1, 0.6));
    TS_ASSERT(!uiv.isAllInputValid());
    TS_ASSERT_EQUALS(uiv.generateErrorMessage(), "Please correct the following:\n\nBinning ranges must be non-zero.");
  }

  void test_reverseRangeRebin()
  {
    UserInputValidator uiv;
    TS_ASSERT(!uiv.checkBins(1.8, 0.1, 0.6));
    TS_ASSERT(!uiv.isAllInputValid());
    TS_ASSERT_EQUALS(uiv.generateErrorMessage(), "Please correct the following:\n\nThe start of a binning range must be less than the end.");
  }

  void test_binsNotFactorsRebin()
  {
    UserInputValidator uiv;
    TS_ASSERT(!uiv.checkBins(0.0, 0.2, 0.7));
    TS_ASSERT(!uiv.isAllInputValid());
    TS_ASSERT_EQUALS(uiv.generateErrorMessage(), "Please correct the following:\n\nBin width must allow for even splitting of the range.");
  }

  void test_validRange()
  {
    UserInputValidator uiv;
    std::pair<double, double> range(1, 5);
    TS_ASSERT(uiv.checkValidRange("test range", range));
    TS_ASSERT(uiv.isAllInputValid());
  }

  void test_invalidRangeReversed()
  {
    UserInputValidator uiv;
    std::pair<double, double> range(10, 5);
    TS_ASSERT(!uiv.checkValidRange("test range", range));
    TS_ASSERT(!uiv.isAllInputValid());
    TS_ASSERT_EQUALS(uiv.generateErrorMessage(), "Please correct the following:\n\nThe start of test range must be less than the end.");
  }

  void test_invalidRangeZeroWidth()
  {
    UserInputValidator uiv;
    std::pair<double, double> range(5, 5);
    TS_ASSERT(!uiv.checkValidRange("test range", range));
    TS_ASSERT(!uiv.isAllInputValid());
    TS_ASSERT_EQUALS(uiv.generateErrorMessage(), "Please correct the following:\n\ntest range must have a non-zero width.");
  }

  void test_nonOverlappingRanges()
  {
    UserInputValidator uiv;
    std::pair<double, double> rangeA(1, 5);
    std::pair<double, double> rangeB(6, 10);
    TS_ASSERT(uiv.checkRangesDontOverlap(rangeA, rangeB));
    TS_ASSERT(uiv.isAllInputValid());
  }

  void test_overlappingRanges()
  {
    UserInputValidator uiv;
    std::pair<double, double> rangeA(1, 5);
    std::pair<double, double> rangeB(3, 8);
    TS_ASSERT(!uiv.checkRangesDontOverlap(rangeA, rangeB));
    TS_ASSERT(!uiv.isAllInputValid());
    TS_ASSERT_EQUALS(uiv.generateErrorMessage(), "Please correct the following:\n\nThe ranges must not overlap: [1,5], [3,8].");
  }

  void test_enclosedRange()
  {
    UserInputValidator uiv;
    std::pair<double, double> outer(1, 10);
    std::pair<double, double> inner(3, 8);
    TS_ASSERT(uiv.checkRangeIsEnclosed("outer range", outer, "inner range", inner));
    TS_ASSERT(uiv.isAllInputValid());
  }

  void test_nonEnclosedRange()
  {
    UserInputValidator uiv;
    std::pair<double, double> outer(1, 10);
    std::pair<double, double> inner(3, 15);
    TS_ASSERT(!uiv.checkRangeIsEnclosed("outer range", outer, "inner range", inner));
    TS_ASSERT(!uiv.isAllInputValid());
    TS_ASSERT_EQUALS(uiv.generateErrorMessage(), "Please correct the following:\n\nouter range must completely enclose inner range.");
  }

};

#endif
