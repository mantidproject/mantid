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

};

#endif
