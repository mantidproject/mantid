// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef IMPLICIT_FUNCTION_PARAMETER_PARSER_FACTORY_TEST_H_
#define IMPLICIT_FUNCTION_PARAMETER_PARSER_FACTORY_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>

#include "MantidAPI/ImplicitFunctionParameterParser.h"
#include "MantidAPI/ImplicitFunctionParameterParserFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/WarningSuppressions.h"
#include <boost/shared_ptr.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class ImplicitFunctionParameterParserFactoryTest : public CxxTest::TestSuite {
private:
  class MockImplicitFunctionParameter
      : public Mantid::API::ImplicitFunctionParameter {
  public:
    GNU_DIAG_OFF_SUGGEST_OVERRIDE
    MOCK_CONST_METHOD0(getName, std::string());
    MOCK_CONST_METHOD0(isValid, bool());
    MOCK_CONST_METHOD0(toXMLString, std::string());
    GNU_DIAG_ON_SUGGEST_OVERRIDE
  protected:
    ImplicitFunctionParameter *clone() const override {
      return new MockImplicitFunctionParameter;
    }
  };

  // TODO, use mocking framework instead!
  class MockImplicitFunctionParameterParserA
      : public Mantid::API::ImplicitFunctionParameterParser {
  public:
    Mantid::API::ImplicitFunctionParameter *
    createParameter(Poco::XML::Element *) override {
      return new MockImplicitFunctionParameter;
    }
    void setSuccessorParser(
        Mantid::API::ImplicitFunctionParameterParser *) override {}
  };

  class MockImplicitFunctionParameterParserB
      : public Mantid::API::ImplicitFunctionParameterParser {
  public:
    Mantid::API::ImplicitFunctionParameter *
    createParameter(Poco::XML::Element *) override {
      return new MockImplicitFunctionParameter;
    }
    void setSuccessorParser(
        Mantid::API::ImplicitFunctionParameterParser *) override {}
  };

public:
  void testSetup() {

    Mantid::API::ImplicitFunctionParameterParserFactory::Instance()
        .subscribe<MockImplicitFunctionParameterParserA>(
            "MockImplicitFunctionParameterParserA");
    Mantid::API::ImplicitFunctionParameterParserFactory::Instance()
        .subscribe<MockImplicitFunctionParameterParserB>(
            "MockImplicitFunctionParameterParserB");
  }

  void testGetFirstConcreteInstance() {
    Mantid::API::ImplicitFunctionParameterParser *parser =
        Mantid::API::ImplicitFunctionParameterParserFactory::Instance()
            .createUnwrapped("MockImplicitFunctionParameterParserA");
    MockImplicitFunctionParameterParserA *a =
        dynamic_cast<MockImplicitFunctionParameterParserA *>(parser);
    TSM_ASSERT("The correct implicit implicit function parameter parser type "
               "has not been generated",
               nullptr != a);
    delete parser;
  }

  void testGetSecondConcreteInstance() {
    Mantid::API::ImplicitFunctionParameterParser *parser =
        Mantid::API::ImplicitFunctionParameterParserFactory::Instance()
            .createUnwrapped("MockImplicitFunctionParameterParserB");
    MockImplicitFunctionParameterParserB *b =
        dynamic_cast<MockImplicitFunctionParameterParserB *>(parser);
    TSM_ASSERT("The correct implicit function parameter parser type has not "
               "been generated",
               nullptr != b);
    delete parser;
  }

  void testCreateThrows() {
    TSM_ASSERT_THROWS(
        "Should have thrown exception on use of create rather than "
        "createunwrapped.",
        Mantid::API::ImplicitFunctionParameterParserFactory::Instance().create(
            ""),
        const std::runtime_error &);
  }
};

#endif
