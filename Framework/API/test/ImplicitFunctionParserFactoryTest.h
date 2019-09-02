// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef IMPLICIT_FUNCTION_PARSER_FACTORY_TEST_H_
#define IMPLICIT_FUNCTION_PARSER_FACTORY_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>

#include "MantidAPI/ImplicitFunctionParameter.h"
#include "MantidAPI/ImplicitFunctionParameterParser.h"
#include "MantidAPI/ImplicitFunctionParser.h"
#include "MantidAPI/ImplicitFunctionParserFactory.h"
#include "MantidKernel/ConfigService.h"
#include <boost/shared_ptr.hpp>

class ImplicitFunctionParserFactoryTest : public CxxTest::TestSuite {
private:
  // TODO, use mocking framework instead!
  class MockImplicitFunctionParameterParser
      : public Mantid::API::ImplicitFunctionParameterParser {
  public:
    Mantid::API::ImplicitFunctionParameter *
    createParameter(Poco::XML::Element *) override {
      return nullptr;
    }
    void setSuccessorParser(
        Mantid::API::ImplicitFunctionParameterParser *) override {}
  };

  class MockImplicitFunctionParserA
      : public Mantid::API::ImplicitFunctionParser {
  public:
    MockImplicitFunctionParserA()
        : Mantid::API::ImplicitFunctionParser(
              new MockImplicitFunctionParameterParser) {}

    Mantid::API::ImplicitFunctionBuilder *
    createFunctionBuilder(Poco::XML::Element *) override {
      return nullptr;
    }
    void setSuccessorParser(Mantid::API::ImplicitFunctionParser *) override {}
    void setParameterParser(
        Mantid::API::ImplicitFunctionParameterParser *) override {}
  };

  class MockImplicitFunctionParserB
      : public Mantid::API::ImplicitFunctionParser {
  public:
    MockImplicitFunctionParserB()
        : Mantid::API::ImplicitFunctionParser(
              new MockImplicitFunctionParameterParser) {}

    Mantid::API::ImplicitFunctionBuilder *
    createFunctionBuilder(Poco::XML::Element *) override {
      return nullptr;
    }
    void setSuccessorParser(Mantid::API::ImplicitFunctionParser *) override {}
    void setParameterParser(
        Mantid::API::ImplicitFunctionParameterParser *) override {}
  };

public:
  void testSetup() {
    Mantid::API::ImplicitFunctionParserFactory::Instance()
        .subscribe<MockImplicitFunctionParserA>("MockImplicitFunctionParserA");
    Mantid::API::ImplicitFunctionParserFactory::Instance()
        .subscribe<MockImplicitFunctionParserB>("MockImplicitFunctionParserB");
  }

  void testGetFirstConcreteInstance() {
    Mantid::API::ImplicitFunctionParser *parser =
        Mantid::API::ImplicitFunctionParserFactory::Instance().createUnwrapped(
            "MockImplicitFunctionParserA");
    MockImplicitFunctionParserA *a =
        dynamic_cast<MockImplicitFunctionParserA *>(parser);
    TSM_ASSERT("The correct implicit parserparameter parser type has not been "
               "generated",
               nullptr != a);
    delete parser;
  }

  void testGetSecondConcreteInstance() {
    Mantid::API::ImplicitFunctionParser *parser =
        Mantid::API::ImplicitFunctionParserFactory::Instance().createUnwrapped(
            "MockImplicitFunctionParserB");
    MockImplicitFunctionParserB *b =
        dynamic_cast<MockImplicitFunctionParserB *>(parser);
    TSM_ASSERT("The correct implicit parserparameter parser type has not been "
               "generated",
               nullptr != b);
    delete parser;
  }

  void testCreateThrows() {
    TSM_ASSERT_THROWS(
        "Should have thrown exception on use of create rather than "
        "createunwrapped.",
        Mantid::API::ImplicitFunctionParserFactory::Instance().create(""),
        const std::runtime_error &);
  }
};

#endif
