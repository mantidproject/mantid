#ifndef PROPERTYWITHVALUETEST_H_
#define PROPERTYWITHVALUETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/DataItem.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/PropertyWithValue.h"

using namespace Mantid::Kernel;

class PropertyWithValueTest : public CxxTest::TestSuite {
public:
  static PropertyWithValueTest *createSuite() {
    return new PropertyWithValueTest();
  }
  static void destroySuite(PropertyWithValueTest *suite) { delete suite; }

  PropertyWithValueTest() {
    iProp = new PropertyWithValue<int>("intProp", 1);
    dProp = new PropertyWithValue<double>("doubleProp", 9.99);
    sProp = new PropertyWithValue<std::string>("stringProp", "theValue");
    lProp = new PropertyWithValue<int64_t>("int64Prop", -9876543210987654LL);
    bProp = new PropertyWithValue<OptionalBool>("boolProp", bool(true));
  }

  ~PropertyWithValueTest() override {
    delete iProp;
    delete dProp;
    delete sProp;
    delete lProp;
    delete bProp;
  }

  void testConstructor() {
    // Test that all the base class member variables are correctly assigned to
    TS_ASSERT(!iProp->name().compare("intProp"));
    TS_ASSERT(!iProp->type().compare("number"));
    TS_ASSERT(!iProp->documentation().compare(""));
    TS_ASSERT(typeid(int) == *iProp->type_info());
    TS_ASSERT(iProp->isDefault());

    TS_ASSERT(!dProp->name().compare("doubleProp"));
    TS_ASSERT(!dProp->type().compare("number"));
    TS_ASSERT(!dProp->documentation().compare(""));
    TS_ASSERT(typeid(double) == *dProp->type_info());
    TS_ASSERT(dProp->isDefault());

    TS_ASSERT(!sProp->name().compare("stringProp"));
    TS_ASSERT(!sProp->type().compare("string"));
    TS_ASSERT(!sProp->documentation().compare(""));
    TS_ASSERT(typeid(std::string) == *sProp->type_info());
    TS_ASSERT(sProp->isDefault());

    TS_ASSERT(!lProp->name().compare("int64Prop"));
    TS_ASSERT(!lProp->type().compare("number"));
    TS_ASSERT(!lProp->documentation().compare(""));
    TS_ASSERT(typeid(int64_t) == *lProp->type_info());
    TS_ASSERT(lProp->isDefault());

    TS_ASSERT(!bProp->name().compare("boolProp"));
    TS_ASSERT(!bProp->type().compare("optional boolean"));
    TS_ASSERT(!bProp->documentation().compare(""));
    TS_ASSERT(typeid(OptionalBool) == *bProp->type_info());
    TS_ASSERT(bProp->isDefault());
  }

  void testValue() {
    TS_ASSERT(!iProp->value().compare("1"));
    // Note that some versions of boost::lexical_cast > 1.34 give a string such
    // as
    // 9.9900000000000002 rather than 9.99. Converting back to a double however
    // does
    // still give the correct 9.99.
    TS_ASSERT(!dProp->value().substr(0, 4).compare("9.99"));
    TS_ASSERT(!sProp->value().compare("theValue"));
    TS_ASSERT(!lProp->value().compare("-9876543210987654"));
    TS_ASSERT(!bProp->value().compare("True"));
  }

  void testSizeOfSingleValueProperty() {
    // Test single value properties.
    TS_ASSERT_EQUALS(1, iProp->size());
    TS_ASSERT_EQUALS(1, dProp->size());
    TS_ASSERT_EQUALS(1, sProp->size());
    TS_ASSERT_EQUALS(1, lProp->size());
  }

  void testSizeOfVectorProperty() {
    // Test vector value property.
    std::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    PropertyWithValue<std::vector<int>> *pv =
        new PropertyWithValue<std::vector<int>>("some_array", v);
    TS_ASSERT_EQUALS(int(v.size()), pv->size());

    delete pv;
  }

  /*
  For multifile property [[a, b], c], we should be adding a and b and therefore
  the size of the property is 2.
  */
  void testSizeOfVectorOfVectorProperty() {
    using VecInt = std::vector<int>;
    using VecVecInt = std::vector<VecInt>;
    // Test vector value property.
    VecVecInt v;
    v.push_back(VecInt(1, 0));
    v.push_back(VecInt(2, 0));
    v.push_back(VecInt(1, 0));
    PropertyWithValue<VecVecInt> *pv =
        new PropertyWithValue<VecVecInt>("some_vec_vec_int", v);
    TSM_ASSERT_EQUALS(
        "The size of the nested vectors should not be taken into account.",
        int(v.size()), pv->size());

    delete pv;
  }

  void testSetValue() {
    PropertyWithValue<int> i("test", 1);
    TS_ASSERT_EQUALS(i.setValue("10"), "");
    TS_ASSERT_EQUALS(i, 10);
    TS_ASSERT_EQUALS(
        i.setValue("9.99"),
        "Could not set property test. Can not convert \"9.99\" to " + i.type());
    TS_ASSERT_EQUALS(
        i.setValue("garbage"),
        "Could not set property test. Can not convert \"garbage\" to " +
            i.type());

    PropertyWithValue<double> d("test", 5.55);
    TS_ASSERT_EQUALS(d.setValue("-9.99"), "");
    TS_ASSERT_EQUALS(d, -9.99);
    TS_ASSERT_EQUALS(d.setValue("0"), "");
    TS_ASSERT_EQUALS(d, 0);
    TS_ASSERT_EQUALS(
        d.setValue("garbage"),
        "Could not set property test. Can not convert \"garbage\" to " +
            d.type());

    PropertyWithValue<std::string> s("test", "test");
    TS_ASSERT_EQUALS(s.setValue("-9.99"), "");
    TS_ASSERT_EQUALS(s.operator()(), "-9.99");
    TS_ASSERT_EQUALS(s.setValue("0"), "");
    TS_ASSERT_EQUALS(s.operator()(), "0");
    TS_ASSERT_EQUALS(s.setValue("it works"), "");
    TS_ASSERT_EQUALS(s.operator()(), "it works");

    PropertyWithValue<int64_t> l("test", 1);
    TS_ASSERT_EQUALS(l.setValue("10"), "");
    TS_ASSERT_EQUALS(l, 10);
    TS_ASSERT_EQUALS(l.setValue("1234567890123456"), "");
    TS_ASSERT_EQUALS(l, 1234567890123456LL);
    TS_ASSERT_EQUALS(
        l.setValue("9.99"),
        "Could not set property test. Can not convert \"9.99\" to " + l.type());
    TS_ASSERT_EQUALS(
        l.setValue("garbage"),
        "Could not set property test. Can not convert \"garbage\" to " +
            l.type());
  }

private:
  class DataObjectOne : public DataItem {
    const std::string &getName() const override { return m_name; };
    const std::string id() const override { return "DataObjectOne"; }
    bool threadSafe() const override { return true; }
    const std::string toString() const override { return m_name; }

  private:
    std::string m_name{"MyName1"};
  };

  class DataObjectTwo : public DataItem {
    const std::string &getName() const override { return m_name; };
    const std::string id() const override { return "DataObjectTwo"; }
    bool threadSafe() const override { return true; }
    const std::string toString() const override { return m_name; }

  private:
    std::string m_name{"MyName2"};
  };

public:
  void testGetDefault() {
    PropertyWithValue<std::string> s("defau=theDef", "theDef");
    TS_ASSERT_EQUALS(s.getDefault(), "theDef");
    TS_ASSERT_EQUALS(s.setValue("somethingElse"), "");
    TS_ASSERT_EQUALS(s.getDefault(), "theDef");

    PropertyWithValue<int> i("defau1", 3);
    TS_ASSERT_EQUALS(i.getDefault(), "3");
    TS_ASSERT_EQUALS(i.setValue("5"), "");
    TS_ASSERT_EQUALS(i.getDefault(), "3");
    TS_ASSERT_EQUALS(
        i.setValue("garbage"),
        "Could not set property defau1. Can not convert \"garbage\" to " +
            i.type());
    TS_ASSERT_EQUALS(i.getDefault(), "3");

    PropertyWithValue<int64_t> l("defau1", 987987987987LL);
    TS_ASSERT_EQUALS(l.getDefault(), "987987987987");
    TS_ASSERT_EQUALS(l.setValue("5"), "");
    TS_ASSERT_EQUALS(l.getDefault(), "987987987987");
    TS_ASSERT_EQUALS(
        l.setValue("garbage"),
        "Could not set property defau1. Can not convert \"garbage\" to " +
            l.type());
    TS_ASSERT_EQUALS(l.getDefault(), "987987987987");

    // Note that some versions of boost::lexical_cast > 1.34 give a string such
    // as
    // 9.9900000000000002 rather than 9.99. Converting back to a double however
    // does
    // still give the correct 9.99.
    PropertyWithValue<double> d("defau3.33", 3.33);
    TS_ASSERT_EQUALS(d.getDefault().substr(0, 4), "3.33");
    TS_ASSERT_EQUALS(d.setValue("1.6"), "");
    TS_ASSERT_EQUALS(d.getDefault().substr(0, 4), "3.33");
    TS_ASSERT_EQUALS(
        d.setValue("garbage"),
        "Could not set property defau3.33. Can not convert \"garbage\" to " +
            d.type());
    TS_ASSERT_EQUALS(d.getDefault().substr(0, 4), "3.33");
  }

  void testCopyConstructor() {
    PropertyWithValue<int> i = *iProp;
    TS_ASSERT(!i.name().compare("intProp"));
    TS_ASSERT(!i.documentation().compare(""));
    TS_ASSERT(typeid(int) == *i.type_info());
    TS_ASSERT(i.isDefault());
    TS_ASSERT_EQUALS(i, 1);

    PropertyWithValue<double> d = *dProp;
    TS_ASSERT(!d.name().compare("doubleProp"));
    TS_ASSERT(!d.documentation().compare(""));
    TS_ASSERT(typeid(double) == *d.type_info());
    TS_ASSERT(d.isDefault());
    TS_ASSERT_EQUALS(d, 9.99);

    PropertyWithValue<std::string> s = *sProp;
    TS_ASSERT(!s.name().compare("stringProp"));
    TS_ASSERT(!s.documentation().compare(""));
    TS_ASSERT(typeid(std::string) == *s.type_info());
    TS_ASSERT(s.isDefault());
    TS_ASSERT_EQUALS(sProp->operator()(), "theValue");

    PropertyWithValue<int64_t> l = *lProp;
    TS_ASSERT(!lProp->name().compare("int64Prop"));
    TS_ASSERT(!lProp->documentation().compare(""));
    TS_ASSERT(typeid(int64_t) == *lProp->type_info());
    TS_ASSERT(lProp->isDefault());
    TS_ASSERT_EQUALS(l, -9876543210987654LL);
  }

  void testCopyAssignmentOperator() {
    PropertyWithValue<int> i("Prop1", 5);
    i = *iProp;
    TS_ASSERT(!i.name().compare("Prop1"));
    TS_ASSERT(!i.documentation().compare(""));
    TS_ASSERT(!i.isDefault());
    TS_ASSERT_EQUALS(i, 1);

    PropertyWithValue<double> d("Prop2", 5.5);
    d = *dProp;
    TS_ASSERT(!d.name().compare("Prop2"));
    TS_ASSERT(!d.documentation().compare(""));
    TS_ASSERT(!d.isDefault());
    TS_ASSERT_EQUALS(d, 9.99);

    PropertyWithValue<std::string> s("Prop3", "test");
    s = *sProp;
    TS_ASSERT(!s.name().compare("Prop3"));
    TS_ASSERT(!s.documentation().compare(""));
    TS_ASSERT(!s.isDefault());
    TS_ASSERT_EQUALS(sProp->operator()(), "theValue");

    PropertyWithValue<int64_t> l("Prop4", 5);
    l = *lProp;
    TS_ASSERT(!l.name().compare("Prop4"));
    TS_ASSERT(!l.documentation().compare(""));
    TS_ASSERT(!l.isDefault());
    TS_ASSERT_EQUALS(l, -9876543210987654LL);
  }

  void testAssignmentOperator() {
    PropertyWithValue<int> i("Prop1", 5);
    TS_ASSERT_EQUALS(i = 2, 2);
    TS_ASSERT(!i.isDefault());
    i = 5;
    TS_ASSERT(i.isDefault());

    PropertyWithValue<double> d("Prop2", 5.5);
    TS_ASSERT_EQUALS(d = 7.77, 7.77);
    TS_ASSERT(!d.isDefault());
    d = 5.5;
    TS_ASSERT(d.isDefault());

    PropertyWithValue<std::string> s("Prop3", "testing");
    s = "test";
    TS_ASSERT_EQUALS(s.operator()(), "test");
    TS_ASSERT(!s.isDefault());
    s = "testing";
    TS_ASSERT(i.isDefault());

    PropertyWithValue<int64_t> l("Prop4", 987987987987LL);
    TS_ASSERT_EQUALS(l = 2, 2);
    TS_ASSERT(!l.isDefault());
    l = 987987987987LL;
    TS_ASSERT(l.isDefault());

    PropertyWithValue<int> ii("Prop1.1", 6);
    i = ii = 10;
    TS_ASSERT_EQUALS(ii, 10);
    TS_ASSERT_EQUALS(i, 10);

    PropertyWithValue<double> dd("Prop2.2", 6.5);
    d = dd = 1.111;
    TS_ASSERT_EQUALS(dd, 1.111);
    TS_ASSERT_EQUALS(d, 1.111);

    PropertyWithValue<std::string> ss("Prop3.3", "testing2");
    s = ss = "tested";
    TS_ASSERT_EQUALS(ss.operator()(), "tested");
    TS_ASSERT_EQUALS(s.operator()(), "tested");

    PropertyWithValue<int64_t> ll("Prop4.4", 6);
    l = ll = 789789789789LL;
    TS_ASSERT_EQUALS(ll, 789789789789LL);
    TS_ASSERT_EQUALS(l, 789789789789LL);
  }

  void testOperatorBrackets() {
    TS_ASSERT_EQUALS(iProp->operator()(), 1);
    TS_ASSERT_EQUALS(dProp->operator()(), 9.99);
    TS_ASSERT_EQUALS(sProp->operator()(), "theValue");
    TS_ASSERT_EQUALS(lProp->operator()(), -9876543210987654LL);
  }

  void testPlusEqualOperator() {
    std::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    PropertyWithValue<std::vector<int>> *pv =
        new PropertyWithValue<std::vector<int>>("some_array", v);
    PropertyWithValue<std::vector<int>> *pv2 =
        new PropertyWithValue<std::vector<int>>("some_array", v);
    (*pv) += pv2;
    TS_ASSERT_EQUALS(pv->value(), "1,2,3,1,2,3");
    delete pv;
    delete pv2;
  }

  void testPlusEqualOperatorOnYourself() {
    std::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    PropertyWithValue<std::vector<int>> *pv =
        new PropertyWithValue<std::vector<int>>("some_array", v);
    (*pv) += pv;
    TS_ASSERT_EQUALS(pv->value(), "1,2,3,1,2,3");
    delete pv;
  }

  void testOperatorNothing() {
    int i = *iProp;
    TS_ASSERT_EQUALS(i, 1);
    double d = *dProp;
    TS_ASSERT_EQUALS(d, 9.99);
    std::string str = *sProp;
    TS_ASSERT(!str.compare("theValue"));
    int64_t l = *lProp;
    TS_ASSERT_EQUALS(l, -9876543210987654LL);
  }

  void testAllowedValues() {
    TS_ASSERT(iProp->allowedValues().empty());
    TS_ASSERT(dProp->allowedValues().empty());
    TS_ASSERT(sProp->allowedValues().empty());
    TS_ASSERT(lProp->allowedValues().empty());
    TS_ASSERT(!bProp->allowedValues().empty())
    // Tests using a ListValidator are below
  }

  void testCasting() {
    TS_ASSERT_DIFFERS(dynamic_cast<Property *>(iProp),
                      static_cast<Property *>(nullptr));
    PropertyWithValue<int> i("Prop1", 5);
    Property *p = dynamic_cast<Property *>(&i);
    TS_ASSERT(!p->name().compare("Prop1"));
    TS_ASSERT(!p->value().compare("5"));
    TS_ASSERT_EQUALS(p->setValue("10"), "");
    TS_ASSERT(!p->value().compare("10"));
    TS_ASSERT_EQUALS(i, 10);

    TS_ASSERT_DIFFERS(dynamic_cast<Property *>(dProp),
                      static_cast<Property *>(nullptr));
    PropertyWithValue<double> d("Prop2", 5.5);
    Property *pp = dynamic_cast<Property *>(&d);
    TS_ASSERT(!pp->name().compare("Prop2"));
    TS_ASSERT(!pp->value().compare("5.5"));
    TS_ASSERT_EQUALS(pp->setValue("7.777"), "");
    // Note that some versions of boost::lexical_cast > 1.34 give a string such
    // as
    // 9.9900000000000002 rather than 9.99. Converting back to a double however
    // does
    // still give the correct 9.99.
    TS_ASSERT(!pp->value().substr(0, 5).compare("7.777"));
    TS_ASSERT_EQUALS(d, 7.777);

    TS_ASSERT_DIFFERS(dynamic_cast<Property *>(sProp),
                      static_cast<Property *>(nullptr));
    PropertyWithValue<std::string> s("Prop3", "testing");
    Property *ppp = dynamic_cast<Property *>(&s);
    TS_ASSERT(!ppp->name().compare("Prop3"));
    TS_ASSERT(!ppp->value().compare("testing"));
    TS_ASSERT_EQUALS(ppp->setValue("newValue"), "");
    TS_ASSERT(!ppp->value().compare("newValue"));
    TS_ASSERT_EQUALS(s.operator()(), "newValue");

    TS_ASSERT_DIFFERS(dynamic_cast<Property *>(lProp),
                      static_cast<Property *>(nullptr));
    PropertyWithValue<int64_t> l("Prop4", 789789789789LL);
    Property *pppp = dynamic_cast<Property *>(&l);
    TS_ASSERT(!pppp->name().compare("Prop4"));
    TS_ASSERT(!pppp->value().compare("789789789789"));
    TS_ASSERT_EQUALS(pppp->setValue("10"), "");
    TS_ASSERT(!pppp->value().compare("10"));
    TS_ASSERT_EQUALS(l, 10);
  }

  void testMandatoryValidator() {
    PropertyWithValue<std::string> p(
        "test", "", boost::make_shared<MandatoryValidator<std::string>>());
    TS_ASSERT_EQUALS(p.isValid(), "A value must be entered for this parameter");
    TS_ASSERT_EQUALS(p.setValue("I'm here"), "");
    TS_ASSERT_EQUALS(p.isValid(), "");
    TS_ASSERT_EQUALS(p.setValue(""),
                     "A value must be entered for this parameter");
    TS_ASSERT_EQUALS(p.value(), "I'm here");
  }

  void testIntBoundedValidator() {
    std::string start("Selected value "), end(")"),
        greaterThan(" is > the upper bound ("),
        lessThan(" is < the lower bound (");

    // int tests
    PropertyWithValue<int> pi("test", 11,
                              boost::make_shared<BoundedValidator<int>>(1, 10));
    TS_ASSERT_EQUALS(pi.isValid(), start + "11" + greaterThan + "10" + end);
    TS_ASSERT_EQUALS(pi.setValue("0"), start + "0" + lessThan + "1" + end);
    TS_ASSERT_EQUALS(pi.value(), "11");
    TS_ASSERT_EQUALS(pi.isValid(), start + "11" + greaterThan + "10" + end);
    TS_ASSERT_EQUALS(pi.setValue("1"), "");
    TS_ASSERT_EQUALS(pi.isValid(), "");
    TS_ASSERT_EQUALS(pi.setValue("10"), "");
    TS_ASSERT_EQUALS(pi.isValid(), "");
    TS_ASSERT_EQUALS(pi.setValue("11"),
                     start + "11" + greaterThan + "10" + end);
    TS_ASSERT_EQUALS(pi.value(), "10");
    TS_ASSERT_EQUALS(pi.isValid(), "");
    std::string errorMsg = pi.setValue("");
    // when the string can't be converted to the correct type we get a system
    // dependent meassage that in this case should look like the string below
    TS_ASSERT_EQUALS(
        errorMsg.find("Could not set property test. Can not convert \"\" to ",
                      0),
        0);

    // double tests
    PropertyWithValue<double> pd(
        "test", 11.0, boost::make_shared<BoundedValidator<double>>(1.0, 10.0));
    TS_ASSERT_EQUALS(pd.isValid(), start + "11" + greaterThan + "10" + end);
    TS_ASSERT_EQUALS(pd.setValue("0.9"), start + "0.9" + lessThan + "1" + end);
    TS_ASSERT_EQUALS(pd.value(), "11");
    TS_ASSERT_EQUALS(pd.isValid(), start + "11" + greaterThan + "10" + end);
    TS_ASSERT_EQUALS(pd.setValue("1"), "");
    TS_ASSERT_EQUALS(pd.isValid(), "");
    TS_ASSERT_EQUALS(pd.setValue("10"), "");
    TS_ASSERT_EQUALS(pd.isValid(), "");
    TS_ASSERT_EQUALS(pd.setValue("10.1"),
                     start + "10.1" + greaterThan + "10" + end);
    TS_ASSERT_EQUALS(pd.value(), "10");
    TS_ASSERT_EQUALS(pd.isValid(), "");

    // string tests
    PropertyWithValue<std::string> ps(
        "test", "",
        boost::make_shared<BoundedValidator<std::string>>("B", "T"));
    TS_ASSERT_EQUALS(ps.isValid(), start + "" + lessThan + "B" + end);
    TS_ASSERT_EQUALS(ps.setValue("AZ"), start + "AZ" + lessThan + "B" + end);
    TS_ASSERT_EQUALS(ps.value(), "");
    TS_ASSERT_EQUALS(ps.isValid(), start + "" + lessThan + "B" + end);
    TS_ASSERT_EQUALS(ps.setValue("B"), "");
    TS_ASSERT_EQUALS(ps.isValid(), "");
    TS_ASSERT_EQUALS(ps.setValue("T"), "");
    TS_ASSERT_EQUALS(ps.isValid(), "");
    TS_ASSERT_EQUALS(ps.setValue("TA"), start + "TA" + greaterThan + "T" + end);
    TS_ASSERT_EQUALS(ps.value(), "T");
    TS_ASSERT_EQUALS(ps.isValid(), "");

    // int64 tests
    PropertyWithValue<int64_t> pl(
        "test", 987987987987LL,
        boost::make_shared<BoundedValidator<int64_t>>(0, 789789789789LL));
    TS_ASSERT_EQUALS(pl.isValid(), start + "987987987987" + greaterThan +
                                       "789789789789" + end);
    TS_ASSERT_EQUALS(pl.setValue("-1"), start + "-1" + lessThan + "0" + end);
    TS_ASSERT_EQUALS(pl.value(), "987987987987");
    TS_ASSERT_EQUALS(pl.setValue("0"), "");
    TS_ASSERT_EQUALS(pl.isValid(), "");
    TS_ASSERT_EQUALS(pl.setValue("789789789789"), "");
    TS_ASSERT_EQUALS(pl.isValid(), "");
    TS_ASSERT_EQUALS(pl.setValue("789789789790"), start + "789789789790" +
                                                      greaterThan +
                                                      "789789789789" + end);
    TS_ASSERT_EQUALS(pl.value(), "789789789789");
  }

  void testListValidator() {
    std::string start("The value '"),
        end("' is not in the list of allowed values");

    std::vector<std::string> empt, vec;
    PropertyWithValue<std::string> empty(
        "test", "", boost::make_shared<StringListValidator>(empt));
    TS_ASSERT_EQUALS(empty.isValid(), "Select a value");
    vec.emplace_back("one");
    vec.emplace_back("two");
    PropertyWithValue<std::string> p(
        "test", "", boost::make_shared<StringListValidator>(vec));
    TS_ASSERT_EQUALS(p.isValid(), "Select a value");
    TS_ASSERT_EQUALS(p.setValue("one"), "");
    TS_ASSERT_EQUALS(p.isValid(), "");
    TS_ASSERT_EQUALS(p.setValue("two"), "");
    TS_ASSERT_EQUALS(p.isValid(), "");
    TS_ASSERT_EQUALS(
        p.setValue("three"),
        "The value \"three\" is not in the list of allowed values");
    TS_ASSERT_EQUALS(p.value(), "two");
    TS_ASSERT_EQUALS(p.isValid(), "");
    std::vector<std::string> s;
    TS_ASSERT_THROWS_NOTHING(s = p.allowedValues());
    TS_ASSERT_EQUALS(s.size(), 2);
    TS_ASSERT(std::find(s.begin(), s.end(), "one") != s.end())
    TS_ASSERT(std::find(s.begin(), s.end(), "two") != s.end())
  }

  void testIsDefault() {
    TS_ASSERT_EQUALS(iProp->setValue("1"), "");
    // 1 is was the initial value and so the next test should pass
    TS_ASSERT(iProp->isDefault());
    TS_ASSERT_EQUALS(iProp->setValue("2"), "");
    TS_ASSERT(!iProp->isDefault());
  }

  // class A
  //{
  // public:
  //  virtual A& fun(const A& rhs)
  //  {
  //    std::cout << "fun() called from class A\n";
  //  }
  //};
  //
  // template <typename T>
  // class B : public A
  //{
  // public:
  //  B(T value) : m_value(value)
  //  {
  //
  //  }
  //
  //  virtual A& fun(const A& rhs)
  //  {
  //    std::cout << "fun() called from class B<T>. I contain " << m_value << "
  //    and the parameter contains " << rhs.m_value << "\n";
  //  }
  //  T m_value;
  //};
  //
  //  void testTemplates()
  //  {
  //    std::cout << "\n\n";
  //    A myA;
  //    B<int> myB(12);
  //    myA.fun(myA);
  //    myB.fun(myB);
  //
  //    A * myAptr = new A();
  //    A * myBptr = new B<int>(23);
  //
  //    myAptr->fun(*myAptr);
  //    myBptr->fun(*myBptr);
  //
  //  }

  void testAdditionOperator() {
    int i;
    double d;
    Property *p1;
    Property *p2;

    // --- Numbers are added together ----
    p1 = new PropertyWithValue<double>("Prop1", 12.0);
    p2 = new PropertyWithValue<double>("Prop1", 23.0);
    (*p1) += p2;
    PropertyWithValue<double> *pd =
        dynamic_cast<PropertyWithValue<double> *>(p1);
    d = *pd;
    TS_ASSERT_EQUALS(d, 35.0);
    delete p1;
    delete p2;

    p1 = new PropertyWithValue<int>("Prop1", 34);
    p2 = new PropertyWithValue<int>("Prop1", 62);
    (*p1) += p2;
    PropertyWithValue<int> *pi = dynamic_cast<PropertyWithValue<int> *>(p1);
    i = *pi;
    TS_ASSERT_EQUALS(i, 96);
    delete p1;
    delete p2;

    // --- Vectors are appennded together ----
    std::vector<int> v1{1, 2, 3, 4, 5, 6}, v2;
    p1 = new PropertyWithValue<std::vector<int>>("Prop1", v1);
    p2 = new PropertyWithValue<std::vector<int>>("Prop1", v2);
    (*p1) += p2;
    PropertyWithValue<std::vector<int>> *pvi =
        dynamic_cast<PropertyWithValue<std::vector<int>> *>(p1);
    std::vector<int> v3 = *pvi;
    TS_ASSERT_EQUALS(v3.size(), 6);
    delete p1;
    delete p2;
  }

  void test_string_property_alias() {
    // system("pause");
    std::array<std::string, 2> allowedValues = {{"Hello", "World"}};
    std::map<std::string, std::string> alias{{"1", "Hello"}, {"0", "World"}};
    auto validator =
        boost::make_shared<ListValidator<std::string>>(allowedValues, alias);
    PropertyWithValue<std::string> prop("String", "", validator);
    TS_ASSERT_THROWS_NOTHING(prop = "Hello");
    std::string value = prop;
    TS_ASSERT_EQUALS(value, "Hello");

    TS_ASSERT_THROWS(prop = "Mantid", std::invalid_argument);

    TS_ASSERT_THROWS_NOTHING(prop = "1");
    value = prop;
    TS_ASSERT_EQUALS(value, "Hello");

    TS_ASSERT_THROWS_NOTHING(prop = "0");
    value = prop;
    TS_ASSERT_EQUALS(value, "World");
  }

  void test_optional_bool_propert_made_from_optional_bool() {
    OptionalBool inputValue(OptionalBool::False);
    PropertyWithValue<OptionalBool> prop("bool_property", inputValue);
    OptionalBool heldValue = prop();
    TS_ASSERT_EQUALS(heldValue, inputValue);
  }

  void test_optional_bool_to_setValue() {

    std::string input = OptionalBool::StrTrue;
    PropertyWithValue<OptionalBool> property("myproperty", OptionalBool::Unset,
                                             Direction::Input);
    property.setValue(input);
  }

  void test_optional_bool_allowed_values() {
    PropertyWithValue<OptionalBool> property("myproperty", OptionalBool::Unset,
                                             Direction::Input);

    auto values = property.allowedValues();
    auto possibilities = OptionalBool::strToEmumMap();
    TSM_ASSERT_EQUALS("3 states allowed", possibilities.size(), values.size());
    for (auto &value : values) {
      TSM_ASSERT("value not a known state",
                 possibilities.find(value) != possibilities.end());
    }
  }

  void test_trimming_string_property() {
    std::string stringWithWhitespace = "  value with whitespace\t\t \r\n";
    std::string trimmedStringWithWhitespace = "value with whitespace";
    sProp->setValue(stringWithWhitespace);
    TSM_ASSERT_EQUALS("Input value has not been trimmed", sProp->value(),
                      trimmedStringWithWhitespace);

    // turn trimming off
    sProp->setAutoTrim(false);
    TSM_ASSERT_EQUALS("Auto trim is not turned off", sProp->autoTrim(), false);

    sProp->setValue(stringWithWhitespace);
    TSM_ASSERT_EQUALS("Input value has been trimmed when it should not",
                      sProp->value(), stringWithWhitespace);

    // turn trimming on
    sProp->setAutoTrim(true);
    TSM_ASSERT_EQUALS("Auto trim is not turned on", sProp->autoTrim(), true);

    // test assignment
    *sProp = stringWithWhitespace;
    TSM_ASSERT_EQUALS("Assignment input value has not been trimmed",
                      sProp->value(), trimmedStringWithWhitespace);

    // test assignment with string literal
    *sProp = "  value with whitespace\t\t \r\n";
    TSM_ASSERT_EQUALS("Assignment string literal has not been trimmed",
                      sProp->value(), trimmedStringWithWhitespace);
  }

  void test_trimming_integer_property() {
    std::string stringWithWhitespace = "  1243\t\t \r\n";
    std::string trimmedStringWithWhitespace = "1243";
    iProp->setValue(stringWithWhitespace);
    TSM_ASSERT_EQUALS("Input value has not been trimmed", iProp->value(),
                      trimmedStringWithWhitespace);

    // turn trimming off
    iProp->setAutoTrim(false);
    TSM_ASSERT_EQUALS("Auto trim is not turned off", iProp->autoTrim(), false);

    iProp->setValue(stringWithWhitespace);
    TSM_ASSERT_EQUALS("Input value should still appear trimmed for an integer",
                      iProp->value(), trimmedStringWithWhitespace);

    // turn trimming on
    iProp->setAutoTrim(true);
    TSM_ASSERT_EQUALS("Auto trim is not turned on", iProp->autoTrim(), true);
  }

private:
  PropertyWithValue<int> *iProp;
  PropertyWithValue<double> *dProp;
  PropertyWithValue<std::string> *sProp;
  PropertyWithValue<int64_t> *lProp;
  PropertyWithValue<OptionalBool> *bProp;
};

#endif /*PROPERTYWITHVALUETEST_H_*/
