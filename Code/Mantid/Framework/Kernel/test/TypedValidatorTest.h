#ifndef MANTID_KERNEL_TYPEDVALIDATORTEST_H_
#define MANTID_KERNEL_TYPEDVALIDATORTEST_H_

#include "MantidKernel/TypedValidator.h"
#include "MantidKernel/DataItem.h"
#include <cxxtest/TestSuite.h>

namespace
{
#define DECLARE_TEST_VALIDATOR(ClassName, HeldType) \
  class ClassName : public Mantid::Kernel::TypedValidator<HeldType>\
  {\
  public:\
    Mantid::Kernel::IValidator_sptr clone() const\
    {\
      return boost::make_shared<ClassName>();\
    }\
    std::string checkValidity(const HeldType &) const\
    {\
      return "";\
    }\
  };

  /// Dummy object to hold in a shared_ptr for test
  struct Holder
  {};
  DECLARE_TEST_VALIDATOR(SharedPtrTypedValidator, boost::shared_ptr<Holder>)
  DECLARE_TEST_VALIDATOR(PODTypedValidator, double)
  class FakeDataItem : public Mantid::Kernel::DataItem
  {
  public:
    virtual const std::string id() const { return "FakeDataItem"; }
    virtual const std::string name() const { return "Empty"; }
    virtual bool threadSafe() const { return true; }
    virtual const std::string toString() const { return "FakeDataItem{}"; }
  };
  DECLARE_TEST_VALIDATOR(DataItemSptrTypedValidator, boost::shared_ptr<FakeDataItem>)
}

class TypedValidatorTest : public CxxTest::TestSuite
{
public:

  void test_shared_ptr_is_passed_successfully_to_concrete_validator()
  {
    Mantid::Kernel::IValidator_sptr valueChecker = boost::make_shared<SharedPtrTypedValidator>();
    const boost::shared_ptr<Holder> testPtr(new Holder);


    checkIsValidReturnsEmptyString<boost::shared_ptr<Holder>>(valueChecker, testPtr);
  }

  void test_simple_type_passed_successfully_to_concrete_validator()
  {
    Mantid::Kernel::IValidator_sptr valueChecker = boost::make_shared<PODTypedValidator>();

    checkIsValidReturnsEmptyString<double>(valueChecker, 10.0);
  }

  void test_DataItem_sptr_descendent_is_passed_successfully_to_concrete_validator()
  {
    Mantid::Kernel::IValidator_sptr valueChecker = boost::make_shared<DataItemSptrTypedValidator>();
    boost::shared_ptr<FakeDataItem> fakeData(new FakeDataItem);
    checkIsValidReturnsEmptyString<boost::shared_ptr<FakeDataItem>>(valueChecker, fakeData);
  }

private:
  template <typename HeldType>
  void checkIsValidReturnsEmptyString(const Mantid::Kernel::IValidator_sptr valueChecker, const HeldType & value)
  {
    std::string error;
    TS_ASSERT_THROWS_NOTHING(error = valueChecker->isValid(value));
    TS_ASSERT_EQUALS(error, "");
  }
};


#endif /* MANTID_KERNEL_TYPEDVALIDATORTEST_H_ */
