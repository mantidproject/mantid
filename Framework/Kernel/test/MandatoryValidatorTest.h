#ifndef MANDATORYVALIDATORTEST_H_
#define MANDATORYVALIDATORTEST_H_

#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/OptionalBool.h"
#include <cxxtest/TestSuite.h>
#include <string>

using namespace Mantid::Kernel;

class MandatoryValidatorTest : public CxxTest::TestSuite {
public:
  void testConstructor() {
    TS_ASSERT_THROWS_NOTHING(MandatoryValidator<std::string> nsv)
    TS_ASSERT_THROWS_NOTHING(MandatoryValidator<std::vector<int>> nsv)
    TS_ASSERT_THROWS_NOTHING(MandatoryValidator<std::vector<double>> nsv)
    TS_ASSERT_THROWS_NOTHING(MandatoryValidator<std::vector<std::string>> nsv)
  }

  void testClone() {
    IValidator_sptr v = boost::make_shared<MandatoryValidator<std::string>>();
    IValidator_sptr vv = v->clone();
    TS_ASSERT_DIFFERS(v, vv);
    TS_ASSERT(boost::dynamic_pointer_cast<MandatoryValidator<std::string>>(vv));

    IValidator_sptr i =
        boost::make_shared<MandatoryValidator<std::vector<int>>>();
    IValidator_sptr ii = i->clone();
    TS_ASSERT_DIFFERS(i, ii)
    TS_ASSERT(
        boost::dynamic_pointer_cast<MandatoryValidator<std::vector<int>>>(ii))

    IValidator_sptr d =
        boost::make_shared<MandatoryValidator<std::vector<double>>>();
    IValidator_sptr dd = d->clone();
    TS_ASSERT_DIFFERS(d, dd);
    TS_ASSERT(
        boost::dynamic_pointer_cast<MandatoryValidator<std::vector<double>>>(
            dd));

    IValidator_sptr s =
        boost::make_shared<MandatoryValidator<std::vector<std::string>>>();
    IValidator_sptr ss = s->clone();
    TS_ASSERT_DIFFERS(s, ss);
    TS_ASSERT(boost::dynamic_pointer_cast<
              MandatoryValidator<std::vector<std::string>>>(ss));
  }

  void testMandatoryValidator() {
    MandatoryValidator<std::string> p;
    TS_ASSERT_EQUALS(p.isValid("AZ"), "");
    TS_ASSERT_EQUALS(p.isValid("B"), "");
    TS_ASSERT_EQUALS(p.isValid(""),
                     "A value must be entered for this parameter");
    TS_ASSERT_EQUALS(p.isValid("ta"), "");

    MandatoryValidator<std::vector<int>> i;
    std::vector<int> ivec;
    TS_ASSERT(ivec.empty())
    TS_ASSERT_EQUALS(i.isValid(ivec),
                     "A value must be entered for this parameter")
    ivec.push_back(1);
    TS_ASSERT_EQUALS(i.isValid(ivec), "")

    MandatoryValidator<std::vector<double>> d;
    std::vector<double> dvec;
    TS_ASSERT(dvec.empty())
    TS_ASSERT_EQUALS(d.isValid(dvec),
                     "A value must be entered for this parameter")
    dvec.push_back(1.1);
    TS_ASSERT_EQUALS(d.isValid(dvec), "")

    MandatoryValidator<std::vector<std::string>> s;
    std::vector<std::string> svec;
    TS_ASSERT(svec.empty())
    TS_ASSERT_EQUALS(s.isValid(svec),
                     "A value must be entered for this parameter")
    svec.emplace_back("OK");
    TS_ASSERT_EQUALS(s.isValid(svec), "")

    MandatoryValidator<int> validate_int;
    TS_ASSERT_EQUALS(validate_int.isValid(5), "");
    TS_ASSERT_EQUALS(validate_int.isValid(-10000), "");

    MandatoryValidator<double> validate_dbl;
    TS_ASSERT_EQUALS(validate_dbl.isValid(5.0), "");
    TS_ASSERT_EQUALS(validate_dbl.isValid(-250.0), "");
  }

  void testMandatoryValidatorOptionalBool() {

    MandatoryValidator<OptionalBool> validator;

    OptionalBool defaultValue;
    TS_ASSERT(!validator.isValid(defaultValue).empty());

    OptionalBool notDefaultIsTrue(OptionalBool::True);
    TS_ASSERT(validator.isValid(notDefaultIsTrue).empty());

    OptionalBool notDefaultIsFalse(OptionalBool::False);
    TS_ASSERT(validator.isValid(notDefaultIsFalse).empty());
  }
};

#endif /*MANDATORYVALIDATORTEST_H_*/
