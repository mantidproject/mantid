#ifndef MANTID_KERNEL_UNITLABELTEST_H_
#define MANTID_KERNEL_UNITLABELTEST_H_

#include "MantidKernel/UnitLabel.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using Mantid::Kernel::UnitLabel;

class UnitLabelTest : public CxxTest::TestSuite
{
  class MockLabel: public UnitLabel
  {
  public:
    MockLabel()
    {
      using namespace ::testing;
      ON_CALL(*this, ascii()).WillByDefault(Return(std::string("TextLabel")));
      ON_CALL(*this, utf8()).WillByDefault(Return(std::wstring(L"Utf8TextLabel")));
    }

    MOCK_CONST_METHOD0(ascii, const std::string());
    MOCK_CONST_METHOD0(utf8, const std::wstring());
  };

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static UnitLabelTest *createSuite() { return new UnitLabelTest(); }
  static void destroySuite( UnitLabelTest *suite ) { delete suite; }

  void test_simple_string()
  {
    using namespace ::testing;
    MockLabel label;
    EXPECT_CALL(label, ascii()).Times(::testing::Exactly(1));

    label.ascii();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&label));
  }

  void test_utf8_string()
  {
    using namespace ::testing;
    MockLabel label;
    EXPECT_CALL(label, utf8()).WillOnce(Return(std::wstring(L"")));

    label.utf8();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&label));
  }

  void test_implicit_string_converter_returns_ascii_method()
  {
    using namespace ::testing;
    MockLabel label;
    EXPECT_CALL(label, ascii()).Times(Exactly(1));

    std::string text = label;

    TS_ASSERT(Mock::VerifyAndClearExpectations(&label));
    TS_ASSERT_EQUALS("TextLabel", text);
  }

};


#endif /* MANTID_KERNEL_UNITLABELTEST_H_ */
