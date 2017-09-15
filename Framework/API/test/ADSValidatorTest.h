#ifndef MANTID_API_ADSVALIDATORTEST_H_
#define MANTID_API_ADSVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"
#include <boost/make_shared.hpp>

using Mantid::API::ADSValidator;
using Mantid::API::AnalysisDataService;
using Mantid::API::Workspace;

namespace {
class MockWorkspace : public Workspace {
  const std::string id() const override { return "MockWorkspace"; }
  const std::string toString() const override { return ""; }
  size_t getMemorySize() const override { return 1; }

private:
  MockWorkspace *doClone() const override {
    throw std::runtime_error("Cloning of MockWorkspace is not implemented.");
  }
  MockWorkspace *doCloneEmpty() const override {
    throw std::runtime_error("Cloning of MockWorkspace is not implemented.");
  }
};
}

typedef std::vector<std::string> StringVector;

class ADSValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ADSValidatorTest *createSuite() { return new ADSValidatorTest(); }
  static void destroySuite(ADSValidatorTest *suite) { delete suite; }

  void test_Optional() {
    ADSValidator adsValidator;
    // default is mandatory
    TS_ASSERT_EQUALS(adsValidator.isOptional(), false);
    StringVector sv;
    TS_ASSERT_DIFFERS(adsValidator.isValid(sv), "");

    adsValidator.setOptional(true);
    TS_ASSERT_EQUALS(adsValidator.isOptional(), true);
    TS_ASSERT_EQUALS(adsValidator.isValid(sv), "");
  }

  void test_SingleValue() {
    const std::string wsName = "ADSValidatorTest_w1";
    auto &ads = AnalysisDataService::Instance();
    ads.addOrReplace(wsName, boost::make_shared<MockWorkspace>());

    ADSValidator adsValidator(false);

    StringVector sv;
    sv.push_back(wsName);
    TS_ASSERT_EQUALS(adsValidator.isValid(sv), "");
    sv.push_back(wsName);
    TS_ASSERT_DIFFERS(adsValidator.isValid(sv), "");

    ads.remove(wsName);
  }

  void test_MultipleValue() {
    const std::string ws1Name = "ADSValidatorTest_w1";
    const std::string ws2Name = "ADSValidatorTest_w2";
    const std::string ws3Name = "ADSValidatorTest_w3";
    const std::string wsInvalidName = "ADSValidatorTest_wInvalid";
    auto &ads = AnalysisDataService::Instance();
    ads.addOrReplace(ws1Name, boost::make_shared<MockWorkspace>());
    ads.addOrReplace(ws2Name, boost::make_shared<MockWorkspace>());
    ads.addOrReplace(ws3Name, boost::make_shared<MockWorkspace>());

    ADSValidator adsValidator(true);

    // all valid options
    StringVector sv;
    sv.push_back(ws1Name);
    TS_ASSERT_EQUALS(adsValidator.isValid(sv), "");
    sv.push_back(ws2Name);
    TS_ASSERT_EQUALS(adsValidator.isValid(sv), "");
    sv.push_back(ws3Name);
    TS_ASSERT_EQUALS(adsValidator.isValid(sv), "");

    // invalid ws in string
    sv.push_back(wsInvalidName);
    TS_ASSERT_DIFFERS(adsValidator.isValid(sv), "");

    ads.remove(ws1Name);
    ads.remove(ws2Name);
    ads.remove(ws3Name);
  }

  void test_AllowedValues() {
    const std::string ws1Name = "ADSValidatorTest_w1";
    const std::string ws2Name = "ADSValidatorTest_w2";
    const std::string ws3Name = "ADSValidatorTest_w3";
    const std::string wsInvalidName = "ADSValidatorTest_wInvalid";
    auto &ads = AnalysisDataService::Instance();
    ads.addOrReplace(ws1Name, boost::make_shared<MockWorkspace>());
    ads.addOrReplace(ws2Name, boost::make_shared<MockWorkspace>());
    ads.addOrReplace(ws3Name, boost::make_shared<MockWorkspace>());

    ADSValidator adsValidator(true);

    auto allowedList = adsValidator.allowedValues();
    TS_ASSERT(std::find(allowedList.cbegin(), allowedList.cend(), ws1Name) !=
              allowedList.cend());
    TS_ASSERT(std::find(allowedList.cbegin(), allowedList.cend(), ws2Name) !=
              allowedList.cend());
    TS_ASSERT(std::find(allowedList.cbegin(), allowedList.cend(), ws3Name) !=
              allowedList.cend());
    TS_ASSERT(std::find(allowedList.cbegin(), allowedList.cend(),
                        wsInvalidName) == allowedList.cend());

    ads.remove(ws1Name);
    ads.remove(ws2Name);
    ads.remove(ws3Name);
  }
};

#endif /* MANTID_API_ADSVALIDATORTEST_H_ */
