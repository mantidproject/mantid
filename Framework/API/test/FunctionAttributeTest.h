// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef FUNCTIONATTRIBUTETEST_H_
#define FUNCTIONATTRIBUTETEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;

namespace detail {
class IFT_Funct : public ParamFunction, public IFunction1D {
public:
  IFT_Funct() {
    declareAttribute("DAttr", Attribute(0.0));
    declareAttribute("IAttr", Attribute(0));
    declareAttribute("BAttr", Attribute(false));
    declareAttribute("SAttr", Attribute(""));
    declareAttribute("SQAttr", Attribute("", true));
    declareAttribute("VAttr", Attribute(std::vector<double>()));
    std::vector<double> v(3);
    v[0] = 1;
    v[1] = 2;
    v[2] = 3;
    declareAttribute("VAttr1", Attribute(v));
  }

  std::string name() const override { return "IFT_Funct"; }
  void function1D(double *, const double *, const size_t) const override {}
  void functionDeriv1D(Jacobian *, const double *, const size_t) override {}
};

DECLARE_FUNCTION(IFT_Funct)
} // namespace detail

class FunctionAttributeTest : public CxxTest::TestSuite {
public:
  void test_double_attribute() {
    detail::IFT_Funct f;
    IFunction::Attribute att = f.getAttribute("DAttr");

    TS_ASSERT_EQUALS(att.asDouble(), 0.0);
    TS_ASSERT_EQUALS(att.type(), "double");

    att.setDouble(1.1);
    TS_ASSERT_EQUALS(att.asDouble(), 1.1);

    TS_ASSERT_THROWS(att.setInt(100), const std::runtime_error &);

    att.fromString("0.5");
    TS_ASSERT_EQUALS(att.asDouble(), 0.5);

    f.setAttribute("DAttr", IFunction::Attribute(2.2));
    TS_ASSERT_EQUALS(att.asDouble(), 0.5); // it is a copy
    TS_ASSERT_EQUALS(f.getAttribute("DAttr").asDouble(), 2.2);

    f.setAttributeValue("DAttr", 3.3);
    TS_ASSERT_EQUALS(att.asDouble(), 0.5); // it is a copy
    TS_ASSERT_EQUALS(f.getAttribute("DAttr").asDouble(), 3.3);

    TS_ASSERT(f.getAttribute("DAttr").value() == "3.3" ||
              f.getAttribute("DAttr").value().substr(0, 8) == "3.299999");
  }

  void test_int_attribute() {
    detail::IFT_Funct f;
    IFunction::Attribute att = f.getAttribute("IAttr");

    TS_ASSERT_EQUALS(att.asInt(), 0);
    TS_ASSERT_EQUALS(att.type(), "int");

    att.setInt(1);
    TS_ASSERT_EQUALS(att.asInt(), 1);

    att.fromString("25");
    TS_ASSERT_EQUALS(att.asInt(), 25);

    f.setAttribute("IAttr", IFunction::Attribute(2));
    TS_ASSERT_EQUALS(f.getAttribute("IAttr").asInt(), 2);

    f.setAttributeValue("IAttr", 3);
    TS_ASSERT_EQUALS(f.getAttribute("IAttr").asInt(), 3);

    TS_ASSERT(f.getAttribute("IAttr").value() == "3");
  }

  void test_bool_attribute() {
    detail::IFT_Funct f;
    IFunction::Attribute att = f.getAttribute("BAttr");

    TS_ASSERT_EQUALS(att.asBool(), false);
    TS_ASSERT_EQUALS(att.type(), "bool");

    att.setBool(true);
    TS_ASSERT_EQUALS(att.asBool(), true);

    att.fromString("false");
    TS_ASSERT_EQUALS(att.asBool(), false);

    att.fromString("true");
    TS_ASSERT_EQUALS(att.asBool(), true);
    att.setBool(false);
    att.fromString("TRUE");
    TS_ASSERT_EQUALS(att.asBool(), true);

    f.setAttribute("BAttr", IFunction::Attribute(true));
    TS_ASSERT_EQUALS(f.getAttribute("BAttr").asBool(), true);
    TS_ASSERT(f.getAttribute("BAttr").value() == "true");

    f.setAttributeValue("BAttr", false);
    TS_ASSERT_EQUALS(f.getAttribute("BAttr").asBool(), false);
    TS_ASSERT(f.getAttribute("BAttr").value() == "false");
  }

  void test_string_attribute() {
    detail::IFT_Funct f;
    IFunction::Attribute att = f.getAttribute("SAttr");

    TS_ASSERT_EQUALS(att.asString(), "");
    TS_ASSERT_EQUALS(att.type(), "std::string");

    att.setString("text");
    TS_ASSERT_EQUALS(att.asString(), "text");

    att.fromString("25");
    TS_ASSERT_EQUALS(att.asString(), "25");

    f.setAttribute("SAttr", IFunction::Attribute("Hello"));
    TS_ASSERT_EQUALS(f.getAttribute("SAttr").asString(), "Hello");

    f.setAttributeValue("SAttr", "World");
    TS_ASSERT_EQUALS(f.getAttribute("SAttr").asString(), "World");

    TS_ASSERT(f.getAttribute("SAttr").value() == "World");
  }

  void test_quoted_string_attribute() {
    detail::IFT_Funct f;
    IFunction::Attribute att = f.getAttribute("SQAttr");

    TS_ASSERT_EQUALS(att.asString(), "\"\"");
    TS_ASSERT_EQUALS(att.type(), "std::string");

    att.setString("text");
    TS_ASSERT_EQUALS(att.asString(), "\"text\"");

    att.fromString("25");
    TS_ASSERT_EQUALS(att.asString(), "\"25\"");

    f.setAttribute("SQAttr", IFunction::Attribute("Hello", true));
    TS_ASSERT_EQUALS(f.getAttribute("SQAttr").asString(), "\"Hello\"");

    f.setAttributeValue("SQAttr", "World");
    TS_ASSERT_EQUALS(f.getAttribute("SQAttr").asString(), "\"World\"");

    TS_ASSERT(f.getAttribute("SQAttr").value() == "\"World\"");
  }

  void test_vector_attribute() {
    detail::IFT_Funct f;
    IFunction::Attribute att = f.getAttribute("VAttr");

    TS_ASSERT_EQUALS(att.type(), "std::vector<double>");
    std::vector<double> v = att.asVector();
    TS_ASSERT_EQUALS(v.size(), 0);

    IFunction::Attribute att1 = f.getAttribute("VAttr1");

    std::vector<double> v1 = att1.asVector();
    TS_ASSERT_EQUALS(v1.size(), 3);
    TS_ASSERT_EQUALS(v1[0], 1);
    TS_ASSERT_EQUALS(v1[1], 2);
    TS_ASSERT_EQUALS(v1[2], 3);

    att.setVector(v1);
    v = att.asVector();
    TS_ASSERT_EQUALS(v.size(), 3);
    TS_ASSERT_EQUALS(v[0], 1);
    TS_ASSERT_EQUALS(v[1], 2);
    TS_ASSERT_EQUALS(v[2], 3);

    att.fromString("(3.14, 2.71)");
    v = att.asVector();
    TS_ASSERT_EQUALS(v.size(), 2);
    TS_ASSERT_EQUALS(v[0], 3.14);
    TS_ASSERT_EQUALS(v[1], 2.71);

    att.fromString("(4)");
    v = att.asVector();
    TS_ASSERT_EQUALS(v.size(), 1);
    TS_ASSERT_EQUALS(v[0], 4);

    att.fromString("8");
    v = att.asVector();
    TS_ASSERT_EQUALS(v.size(), 1);
    TS_ASSERT_EQUALS(v[0], 8);

    att.fromString("99 ,100, 101,200");
    v = att.asVector();
    TS_ASSERT_EQUALS(v.size(), 4);
    TS_ASSERT_EQUALS(v[0], 99);
    TS_ASSERT_EQUALS(v[1], 100);
    TS_ASSERT_EQUALS(v[2], 101);
    TS_ASSERT_EQUALS(v[3], 200);

    IFunction::Attribute att2;
    att2 = att;
    const std::vector<double> &v2 = att2.asVector();
    TS_ASSERT_EQUALS(v2.size(), 4);
    TS_ASSERT_EQUALS(v2[0], 99);
    TS_ASSERT_EQUALS(v2[1], 100);
    TS_ASSERT_EQUALS(v2[2], 101);
    TS_ASSERT_EQUALS(v2[3], 200);

    f.setAttribute("VAttr", att1);
    std::vector<double> v3 = f.getAttribute("VAttr").asVector();
    TS_ASSERT_EQUALS(v3.size(), 3);
    TS_ASSERT_EQUALS(v3[0], 1);
    TS_ASSERT_EQUALS(v3[1], 2);
    TS_ASSERT_EQUALS(v3[2], 3);

    f.setAttributeValue("VAttr", v2);
    v3 = f.getAttribute("VAttr").asVector();
    TS_ASSERT_EQUALS(v3.size(), 4);
    TS_ASSERT_EQUALS(v3[0], 99);
    TS_ASSERT_EQUALS(v3[1], 100);
    TS_ASSERT_EQUALS(v3[2], 101);
    TS_ASSERT_EQUALS(v3[3], 200);

    TS_ASSERT(f.getAttribute("VAttr").value() == "(99,100,101,200)");
  }

  void test_factory_creation() {
    auto f = Mantid::API::FunctionFactory::Instance().createInitialized(
        "name=IFT_Funct,DAttr=12.0,IAttr=777,BAttr=true, SAttr= \"Hello "
        "world!\", SQAttr= \"Hello world!\",VAttr=(4,5,6)");
    TS_ASSERT(f);
    TS_ASSERT_EQUALS(f->getAttribute("DAttr").asDouble(), 12.0);
    TS_ASSERT_EQUALS(f->getAttribute("IAttr").asInt(), 777);
    TS_ASSERT_EQUALS(f->getAttribute("BAttr").asBool(), true);
    TS_ASSERT_EQUALS(f->getAttribute("SAttr").asString(), "Hello world!");
    TS_ASSERT_EQUALS(f->getAttribute("SQAttr").asString(), "\"Hello world!\"");
    std::vector<double> v = f->getAttribute("VAttr").asVector();
    TS_ASSERT_EQUALS(v.size(), 3);
    TS_ASSERT_EQUALS(v[0], 4);
    TS_ASSERT_EQUALS(v[1], 5);
    TS_ASSERT_EQUALS(v[2], 6);

    TS_ASSERT_EQUALS(f->asString(), "name=IFT_Funct,BAttr=true,DAttr=12,IAttr="
                                    "777,SAttr=Hello world!,SQAttr=\"Hello "
                                    "world!\",VAttr=(4,5,6),VAttr1=(1,2,3)");
  }

  void test_empty_string_attributes_do_not_show_by_asString() {
    detail::IFT_Funct f;
    TS_ASSERT_EQUALS(
        f.asString(),
        "name=IFT_Funct,BAttr=false,DAttr=0,IAttr=0,VAttr=(),VAttr1=(1,2,3)");
  }
};

#endif /* FUNCTIONATTRIBUTETEST_H_ */
