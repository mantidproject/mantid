// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ARRAYPROPERTYTEST_H_
#define ARRAYPROPERTYTEST_H_

#include "MantidKernel/ArrayProperty.h"
#include <array>
#include <cxxtest/TestSuite.h>
#include <json/value.h>

using namespace Mantid::Kernel;

class ArrayPropertyTest : public CxxTest::TestSuite {
public:
  void setUp() override {
    iProp = new ArrayProperty<int>("intProp");
    dProp = new ArrayProperty<double>("doubleProp");
    sProp = new ArrayProperty<std::string>("stringProp");
  }

  void tearDown() override {
    delete iProp;
    delete dProp;
    delete sProp;
  }

  void testConstructor() {
    TS_ASSERT(!iProp->name().compare("intProp"))
    TS_ASSERT(!iProp->documentation().compare(""))
    TS_ASSERT(typeid(std::vector<int>) == *iProp->type_info())
    TS_ASSERT(iProp->isDefault())
    TS_ASSERT(iProp->operator()().empty());

    TS_ASSERT(!dProp->name().compare("doubleProp"))
    TS_ASSERT(!dProp->documentation().compare(""))
    TS_ASSERT(typeid(std::vector<double>) == *dProp->type_info())
    TS_ASSERT(dProp->isDefault())
    TS_ASSERT(dProp->operator()().empty())

    TS_ASSERT(!sProp->name().compare("stringProp"))
    TS_ASSERT(!sProp->documentation().compare(""))
    TS_ASSERT(typeid(std::vector<std::string>) == *sProp->type_info())
    TS_ASSERT(sProp->isDefault())
    TS_ASSERT(sProp->operator()().empty())

    std::vector<int> i(5, 2);
    ArrayProperty<int> ip("ip", std::move(i));
    TS_ASSERT_EQUALS(ip.operator()().size(), 5)
    TS_ASSERT_EQUALS(ip.operator()()[3], 2)

    std::vector<double> d(4, 6.66);
    ArrayProperty<double> dp("dp", std::move(d));
    TS_ASSERT_EQUALS(dp.operator()().size(), 4)
    TS_ASSERT_EQUALS(dp.operator()()[1], 6.66)

    std::vector<std::string> s(3, "yyy");
    ArrayProperty<std::string> sp("sp", std::move(s));
    TS_ASSERT_EQUALS(sp.operator()().size(), 3)
    TS_ASSERT(!sp.operator()()[2].compare("yyy"))
  }

  void testSize() {
    TS_ASSERT_EQUALS(0, iProp->size());
    TS_ASSERT_EQUALS(0, dProp->size());
    TS_ASSERT_EQUALS(0, sProp->size());

    // Make something bigger and test that.
    Property *a = new ArrayProperty<int>("int_property", "1, 2, 3");
    TS_ASSERT_EQUALS(3, a->size());
    delete a;

    // Test vector of vector.
    // Make it 10 elements long, but
    // should only be the size of the
    // parent vector that is counted.
    std::vector<std::vector<int>> input{{10}};
    Property *b =
        new ArrayProperty<std::vector<int>>("vec_property", std::move(input));
    TS_ASSERT_EQUALS(1, b->size());
    delete b;
  }

  void testConstructorByString() {
    ArrayProperty<int> i("i", "1,2,3");
    TS_ASSERT_EQUALS(i.operator()()[0], 1);
    TS_ASSERT_EQUALS(i.operator()()[1], 2);
    TS_ASSERT_EQUALS(i.operator()()[2], 3);
    TS_ASSERT_EQUALS(i.getDefault(), "1,2,3");
    TS_ASSERT(i.isDefault());

    ArrayProperty<int> i2("i", "-1-1");
    TS_ASSERT_EQUALS(i2.operator()()[0], -1);
    TS_ASSERT_EQUALS(i2.operator()()[1], 0);
    TS_ASSERT_EQUALS(i2.operator()()[2], 1);

    ArrayProperty<int> i3("i", "-1:1");
    TS_ASSERT_EQUALS(i3.operator()()[0], -1);
    TS_ASSERT_EQUALS(i3.operator()()[1], 0);
    TS_ASSERT_EQUALS(i3.operator()()[2], 1);

    ArrayProperty<int> i4("i", "-3--1");
    TS_ASSERT_EQUALS(i4.operator()()[0], -3);
    TS_ASSERT_EQUALS(i4.operator()()[1], -2);
    TS_ASSERT_EQUALS(i4.operator()()[2], -1);

    ArrayProperty<int> i5("i", "-3:-1");
    TS_ASSERT_EQUALS(i5.operator()()[0], -3);
    TS_ASSERT_EQUALS(i5.operator()()[1], -2);
    TS_ASSERT_EQUALS(i5.operator()()[2], -1);

    ArrayProperty<int> i6("i", "-3:0:2");
    TS_ASSERT_EQUALS(i6.operator()()[0], -3);
    TS_ASSERT_EQUALS(i6.operator()()[1], -1);

    // negative step size
    ArrayProperty<int> i7("i", "5:1:-2");
    TS_ASSERT_EQUALS(i7.operator()()[0], 5);
    TS_ASSERT_EQUALS(i7.operator()()[1], 3);
    TS_ASSERT_EQUALS(i7.operator()()[2], 1);

    ArrayProperty<unsigned int> u1("i", "0:2,5");
    TS_ASSERT_EQUALS(u1.operator()()[0], 0);
    TS_ASSERT_EQUALS(u1.operator()()[1], 1);
    TS_ASSERT_EQUALS(u1.operator()()[2], 2);
    TS_ASSERT_EQUALS(u1.operator()()[3], 5);

    ArrayProperty<unsigned int> u2("i", "5,0-2,5");
    TS_ASSERT_EQUALS(u2.operator()()[0], 5);
    TS_ASSERT_EQUALS(u2.operator()()[1], 0);
    TS_ASSERT_EQUALS(u2.operator()()[2], 1);
    TS_ASSERT_EQUALS(u2.operator()()[3], 2);
    TS_ASSERT_EQUALS(u2.operator()()[4], 5);

    ArrayProperty<double> d("d", "7.77,8.88,9.99");
    TS_ASSERT_EQUALS(d.operator()()[0], 7.77)
    TS_ASSERT_EQUALS(d.operator()()[1], 8.88)
    TS_ASSERT_EQUALS(d.operator()()[2], 9.99)

    ArrayProperty<double> d2("d", "-0.15,0.0,0.15");
    TS_ASSERT_EQUALS(d2.operator()()[0], -0.15)
    TS_ASSERT_EQUALS(d2.operator()()[1], 0.0)
    TS_ASSERT_EQUALS(d2.operator()()[2], 0.15)
    // Now we change the values that are set in the indices of d2
    // by using a string
    d2.setValue("0.3,0.1,-0.2");
    TS_ASSERT_EQUALS(d2.operator()()[0], 0.3)
    TS_ASSERT_EQUALS(d2.operator()()[1], 0.1)
    TS_ASSERT_EQUALS(d2.operator()()[2], -0.2)
    TS_ASSERT_EQUALS(d2.value(), "0.3,0.1,-0.2");
    TS_ASSERT(!d2.isDefault());

    ArrayProperty<std::string> s("d", "a,b,c");
    TS_ASSERT(!s.operator()()[0].compare("a"))
    TS_ASSERT(!s.operator()()[1].compare("b"))
    TS_ASSERT(!s.operator()()[2].compare("c"))

    TS_ASSERT_THROWS(ArrayProperty<int> ii("ii", "aa,bb"),
                     const std::bad_cast &)
    TS_ASSERT_THROWS(ArrayProperty<int> ii("ii", "5.5,6.6"),
                     const std::bad_cast &)
    TS_ASSERT_THROWS(ArrayProperty<double> dd("dd", "aa,bb"),
                     const std::bad_cast &)
  }

  void testConstructorByString_long() {
    ArrayProperty<long> prop("long", "0:2,5");
    TS_ASSERT_EQUALS(prop.operator()()[0], 0);
    TS_ASSERT_EQUALS(prop.operator()()[1], 1);
    TS_ASSERT_EQUALS(prop.operator()()[2], 2);
    TS_ASSERT_EQUALS(prop.operator()()[3], 5);
  }

  void testCopyConstructor() {
    ArrayProperty<int> i = *iProp;
    TS_ASSERT(!i.name().compare("intProp"))
    TS_ASSERT(!i.documentation().compare(""))
    TS_ASSERT(typeid(std::vector<int>) == *i.type_info())
    TS_ASSERT(i.isDefault())
    TS_ASSERT(i.operator()().empty())

    ArrayProperty<double> d = *dProp;
    TS_ASSERT(!d.name().compare("doubleProp"))
    TS_ASSERT(!d.documentation().compare(""))
    TS_ASSERT(typeid(std::vector<double>) == *d.type_info())
    TS_ASSERT(d.isDefault())
    TS_ASSERT(d.operator()().empty())

    ArrayProperty<std::string> s = *sProp;
    TS_ASSERT(!s.name().compare("stringProp"))
    TS_ASSERT(!s.documentation().compare(""))
    TS_ASSERT(typeid(std::vector<std::string>) == *s.type_info())
    TS_ASSERT(s.isDefault())
    TS_ASSERT(s.operator()().empty())
  }

  void testValue() {
    std::vector<int> i(3, 3);
    ArrayProperty<int> ip("ip", std::move(i));
    TS_ASSERT(!ip.value().compare("3,3,3"))

    std::vector<double> d(4, 1.23);
    ArrayProperty<double> dp("dp", std::move(d));
    TS_ASSERT(!dp.value().compare("1.23,1.23,1.23,1.23"))

    std::vector<std::string> s(2, "yyy");
    ArrayProperty<std::string> sp("sp", std::move(s));
    TS_ASSERT(!sp.value().compare("yyy,yyy"))
  }

  void testSetValueAndIsDefault() {
    std::string couldnt = "Could not set property ",
                cant = ". Can not convert \"";

    TS_ASSERT_EQUALS(iProp->setValue("1.1,2,2"), couldnt + iProp->name() +
                                                     cant + "1.1,2,2\" to " +
                                                     iProp->type())
    TS_ASSERT(iProp->operator()().empty())
    TS_ASSERT(iProp->isDefault())
    TS_ASSERT_EQUALS(iProp->setValue("aaa,bbb"), couldnt + iProp->name() +
                                                     cant + "aaa,bbb\" to " +
                                                     iProp->type())
    TS_ASSERT(iProp->operator()().empty())
    TS_ASSERT(iProp->isDefault())
    TS_ASSERT_EQUALS(iProp->setValue("1,2,3,4"), "")
    TS_ASSERT_EQUALS(iProp->operator()().size(), 4)
    for (std::size_t i = 0; i < 4; ++i) {
      TS_ASSERT_EQUALS(iProp->operator()()[i], i + 1)
    }
    TS_ASSERT(!iProp->isDefault())
    TS_ASSERT_EQUALS(iProp->setValue(""), "")
    TS_ASSERT(iProp->operator()().empty())
    TS_ASSERT(iProp->isDefault())

    TS_ASSERT_EQUALS(dProp->setValue("aaa,bbb"), couldnt + dProp->name() +
                                                     cant + "aaa,bbb\" to " +
                                                     dProp->type())
    TS_ASSERT(dProp->operator()().empty())
    TS_ASSERT(dProp->isDefault())
    TS_ASSERT_EQUALS(dProp->setValue("1,2"), "")
    TS_ASSERT_EQUALS(dProp->operator()()[1], 2)
    TS_ASSERT(!dProp->isDefault())
    TS_ASSERT_EQUALS(dProp->setValue("1.11,2.22,3.33,4.44"), "")
    TS_ASSERT_EQUALS(dProp->operator()()[0], 1.11)
    TS_ASSERT(!dProp->isDefault())
    TS_ASSERT_EQUALS(dProp->setValue(""), "")
    TS_ASSERT(dProp->operator()().empty())
    TS_ASSERT(dProp->isDefault())

    TS_ASSERT_EQUALS(sProp->setValue("This,is,a,test"), "")
    TS_ASSERT_EQUALS(sProp->operator()()[2], "a")
    TS_ASSERT(!sProp->isDefault())
    TS_ASSERT_EQUALS(sProp->setValue(""), "")
    TS_ASSERT(sProp->operator()().empty())
    TS_ASSERT(sProp->isDefault())
  }

  void test_SetValueFromJson_Accepts_ArrayValues() {
    const std::array<int, 3> testValues{{1, 2, 3}};
    Json::Value arrayValue{Json::arrayValue};
    for (const auto &elem : testValues) {
      arrayValue.append(elem);
    }

    ArrayProperty<int> intProp("i");
    const std::string helpMessage{intProp.setValueFromJson(arrayValue)};

    TS_ASSERT(helpMessage.empty());
    const auto &propValue = intProp();
    TS_ASSERT_EQUALS(testValues.size(), propValue.size());
    for (const auto &elem : testValues) {
      arrayValue.append(elem);
    }
  }

  void test_SetValueFromJson_Returns_Error_StringFor_NonArrayValues() {
    Json::Value dict;
    dict["key"] = "value";
    ArrayProperty<int> intProp("i");
    const std::string helpMessage{intProp.setValueFromJson(dict)};
    TS_ASSERT(!helpMessage.empty());
  }

  void testAssignmentOperator() {
    ArrayProperty<int> i("i");
    TS_ASSERT(i.isDefault())
    std::vector<int> ii(3, 4);
    i = ii;
    TS_ASSERT_EQUALS(i.operator()(), ii);
    TS_ASSERT_EQUALS(i.operator()()[1], 4)
    TS_ASSERT(!i.isDefault())

    ArrayProperty<double> d("d");
    TS_ASSERT(d.isDefault())
    std::vector<double> dd(5, 9.99);
    d = dd;
    TS_ASSERT_EQUALS(d.operator()(), dd);
    TS_ASSERT_EQUALS(d.operator()()[3], 9.99)
    TS_ASSERT(!d.isDefault())

    ArrayProperty<std::string> s("s");
    TS_ASSERT(s.isDefault())
    std::vector<std::string> ss(2, "zzz");
    s = ss;
    TS_ASSERT_EQUALS(s.operator()(), ss)
    TS_ASSERT_EQUALS(s.operator()()[0], "zzz")
    TS_ASSERT(!s.isDefault())
  }

  void testOperatorBrackets() {
    TS_ASSERT(iProp->operator()().empty())
    TS_ASSERT(dProp->operator()().empty())
    TS_ASSERT(sProp->operator()().empty())
  }

  void testOperatorNothing() {
    std::vector<int> i = *iProp;
    TS_ASSERT(i.empty())

    std::vector<double> d(3, 8.8);
    *dProp = d;
    std::vector<double> dd = *dProp;
    for (std::size_t i = 0; i < 3; ++i) {
      TS_ASSERT_EQUALS(dProp->operator()()[i], 8.8)
    }

    std::vector<std::string> s = *sProp;
    TS_ASSERT(s.empty())
  }

  void testCasting() {
    TS_ASSERT_DIFFERS(
        dynamic_cast<PropertyWithValue<std::vector<int>> *>(iProp),
        static_cast<PropertyWithValue<std::vector<int>> *>(nullptr))
    TS_ASSERT_DIFFERS(
        dynamic_cast<PropertyWithValue<std::vector<double>> *>(dProp),
        static_cast<PropertyWithValue<std::vector<double>> *>(nullptr))
    TS_ASSERT_DIFFERS(
        dynamic_cast<PropertyWithValue<std::vector<std::string>> *>(sProp),
        static_cast<PropertyWithValue<std::vector<std::string>> *>(nullptr))

    TS_ASSERT_DIFFERS(dynamic_cast<Property *>(iProp),
                      static_cast<Property *>(nullptr))
    TS_ASSERT_DIFFERS(dynamic_cast<Property *>(dProp),
                      static_cast<Property *>(nullptr))
    TS_ASSERT_DIFFERS(dynamic_cast<Property *>(sProp),
                      static_cast<Property *>(nullptr))
  }

  void testPrettyPrinting() {
    const std::vector<std::string> inputList{
        "1,2,3", "-1,0,1", "356,366,367,368,370,371,372,375", "7,6,5,6,7,8,10",
        "1-9998, 9999, 2000, 20002-29999"};
    const std::vector<std::string> resultList{
        "1-3", "-1-1", "356,366-368,370-372,375", "7,6,5-8,10",
        "1-9999,2000,20002-29999"};

    TSM_ASSERT("Test Failed for vectors of int",
               listShorteningwithType<int>(inputList, resultList));
    TSM_ASSERT("Test Failed for vectors of long",
               listShorteningwithType<long>(inputList, resultList));
    // explicit test for in32_t with matches det_id_t and spec_id_t
    TSM_ASSERT("Test Failed for vectors of int32_t",
               listShorteningwithType<int32_t>(inputList, resultList));

    // unsigned types
    const std::vector<std::string> inputListUnsigned{
        "1,2,3", "356,366,367,368,370,371,372,375", "7,6,5,6,7,8,10",
        "1-9998, 9999, 2000, 20002-29999"};
    const std::vector<std::string> resultListUnsigned{
        "1-3", "356,366-368,370-372,375", "7,6,5-8,10",
        "1-9999,2000,20002-29999"};
    TSM_ASSERT("Test Failed for vectors of unsigned int",
               listShorteningwithType<unsigned int>(inputListUnsigned,
                                                    resultListUnsigned));
    TSM_ASSERT(
        "Test Failed for vectors of size_t",
        listShorteningwithType<size_t>(inputListUnsigned, resultListUnsigned));

    // check shortening does not happen for floating point types
    const std::vector<std::string> inputListFloat{
        "1.0,2.0,3.0",
        "1.0,1.5,2.0,3.0",
        "-1,0,1",
    };
    const std::vector<std::string> resultListFloat{
        "1,2,3",
        "1,1.5,2,3",
        "-1,0,1",
    };
    TSM_ASSERT("Test Failed for vectors of float",
               listShorteningwithType<float>(inputListFloat, resultListFloat));
    TSM_ASSERT("Test Failed for vectors of double",
               listShorteningwithType<double>(inputListFloat, resultListFloat));
  }

  template <typename T>
  bool listShorteningwithType(const std::vector<std::string> &inputList,
                              const std::vector<std::string> &resultList) {

    bool success = true;
    for (size_t i = 0; i < inputList.size(); i++) {
      ArrayProperty<T> listProperty("i", inputList[i]);
      std::string response = listProperty.valueAsPrettyStr(0, true);
      TS_ASSERT_EQUALS(response, resultList[i]);
      if (response != resultList[i]) {
        success = false;
      }
    }
    return success;
  }

private:
  ArrayProperty<int> *iProp;
  ArrayProperty<double> *dProp;
  ArrayProperty<std::string> *sProp;
};

#endif /*ARRAYPROPERTYTEST_H_*/
