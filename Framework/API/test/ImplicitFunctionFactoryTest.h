// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef IMPLICIT_FUNCTION_FACTORY_TEST_H_
#define IMPLICIT_FUNCTION_FACTORY_TEST_H_

#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidAPI/ImplicitFunctionParameter.h"
#include "MantidAPI/ImplicitFunctionParameterParserFactory.h"
#include "MantidAPI/ImplicitFunctionParserFactory.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/WarningSuppressions.h"
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class ImplicitFunctionFactoryTest : public CxxTest::TestSuite {
private:
  class MockImplicitFunctionA : public Mantid::Geometry::MDImplicitFunction {
  public:
    std::string getName() const override { return "MockImplicitFunctionA"; }
    MOCK_METHOD1(isPointContained, bool(const Mantid::coord_t *pPoint));
    MOCK_METHOD1(isPointContained, bool(const std::vector<Mantid::coord_t> &));
    // Unhide base class methods (avoids Intel compiler warning)
    using MDImplicitFunction::isPointContained;
    MOCK_CONST_METHOD0(toXMLString, std::string());
    ~MockImplicitFunctionA() override { ; }
  };

  class MockImplicitFunctionB : public Mantid::Geometry::MDImplicitFunction {
  public:
    MOCK_METHOD1(isPointContained, bool(const Mantid::coord_t *pPoint));
    MOCK_METHOD1(isPointContained, bool(const std::vector<Mantid::coord_t> &));
    // Unhide base class methods (avoids Intel compiler warning)
    using MDImplicitFunction::isPointContained;
    std::string getName() const override { return "MockImplicitFunctionB"; }
    MOCK_CONST_METHOD0(toXMLString, std::string());
    ~MockImplicitFunctionB() override {}
  };

  class MockImplicitFunctionParserA
      : public Mantid::API::ImplicitFunctionParser {
  public:
    MockImplicitFunctionParserA()
        : Mantid::API::ImplicitFunctionParser(
              new MockImplicitFunctionParameterParserA) {}
    Mantid::API::ImplicitFunctionBuilder *
    createFunctionBuilder(Poco::XML::Element *) override {
      return new MockImplicitFunctionBuilderA;
    }
    void setSuccessorParser(
        Mantid::API::ImplicitFunctionParser *successor) override {
      Mantid::API::ImplicitFunctionParser::SuccessorType successor_uptr(
          successor);
      m_successor.swap(successor_uptr);
    }
    void setParameterParser(
        Mantid::API::ImplicitFunctionParameterParser *parser) override {
      Mantid::API::ImplicitFunctionParameterParser::SuccessorType successor(
          parser);
      m_paramParserRoot.swap(successor);
    }
  };

  class MockImplicitFunctionParserB
      : public Mantid::API::ImplicitFunctionParser {
  public:
    MockImplicitFunctionParserB()
        : Mantid::API::ImplicitFunctionParser(
              new MockImplicitFunctionParameterParserB) {}
    Mantid::API::ImplicitFunctionBuilder *
    createFunctionBuilder(Poco::XML::Element *) override {
      return new MockImplicitFunctionBuilderB;
    }
    void setSuccessorParser(
        Mantid::API::ImplicitFunctionParser *successor) override {
      Mantid::API::ImplicitFunctionParser::SuccessorType successor_uptr(
          successor);
      m_successor.swap(successor_uptr);
    }
    void setParameterParser(
        Mantid::API::ImplicitFunctionParameterParser *parser) override {
      Mantid::API::ImplicitFunctionParameterParser::SuccessorType successor(
          parser);
      m_paramParserRoot.swap(successor);
    }
  };

  class MockImplicitFunctionParameterParserA
      : public Mantid::API::ImplicitFunctionParameterParser {
  public:
    MOCK_METHOD1(createParameter, Mantid::API::ImplicitFunctionParameter *(
                                      Poco::XML::Element *functionElement));
    MOCK_METHOD1(setSuccessorParser,
                 void(Mantid::API::ImplicitFunctionParameterParser *successor));
  };

  class MockImplicitFunctionParameterParserB
      : public Mantid::API::ImplicitFunctionParameterParser {
  public:
    MOCK_METHOD1(createParameter, Mantid::API::ImplicitFunctionParameter *(
                                      Poco::XML::Element *functionElement));
    MOCK_METHOD1(setSuccessorParser,
                 void(Mantid::API::ImplicitFunctionParameterParser *successor));
  };
  GNU_DIAG_ON_SUGGEST_OVERRIDE

  class MockImplicitFunctionBuilderA
      : public Mantid::API::ImplicitFunctionBuilder {
  public:
    Mantid::Geometry::MDImplicitFunction *create() const override {
      return new MockImplicitFunctionA;
    }
  };

  class MockImplicitFunctionBuilderB
      : public Mantid::API::ImplicitFunctionBuilder {
  public:
    Mantid::Geometry::MDImplicitFunction *create() const override {
      return new MockImplicitFunctionA;
    }
  };

  // Helper method to generate as simple xml fragment.
  static std::string generateSimpleXML() {
    return std::string("<Function>") + "<Type>MockA1ImplicitFunction</Type>" +
           "<ParameterList>" + "<Parameter>" +
           "<Type>MockA1ImplicitFunctionParameter</Type>" + "<Value></Value>" +
           "</Parameter>" + "</ParameterList>" + "</Function>";
  }

  // Helper method providing a more complex xml fragment.
  static std::string generateComplexXML() {
    return std::string("<Function>") + "<Type>MockA1ImplicitFunction</Type>" +
           "<Function>" + "<Type>MockB1ImplicitFunction</Type>" +
           "<ParameterList>" + "<Parameter>" +
           "<Type>MockB1ImplicitFunctionParameter</Type>" + "<Value></Value>" +
           "</Parameter>" + "</ParameterList>" + "</Function>" +
           "<ParameterList>" + "<Parameter>" +
           "<Type>MockA1ImplicitFunctionParameter</Type>" + "<Value></Value>" +
           "</Parameter>" + "</ParameterList>" + "</Function>";
  }

public:
  void testSetup() {
    using namespace Mantid::Kernel;
    Mantid::API::ImplicitFunctionFactory::Instance()
        .subscribe<testing::NiceMock<MockImplicitFunctionA>>(
            "MockA1ImplicitFunction"); // No warnings if used.
    Mantid::API::ImplicitFunctionFactory::Instance()
        .subscribe<testing::NiceMock<MockImplicitFunctionB>>(
            "MockB1ImplicitFunction"); // Emit warnings if used.
    Mantid::API::ImplicitFunctionParameterParserFactory::Instance()
        .subscribe<testing::NiceMock<MockImplicitFunctionParameterParserA>>(
            "MockA1ImplicitFunctionParameterParser"); // No warnings if used.
    Mantid::API::ImplicitFunctionParameterParserFactory::Instance()
        .subscribe<testing::NiceMock<MockImplicitFunctionParameterParserB>>(
            "MockB1ImplicitFunctionParameterParser"); // Emit warnings if used.
    Mantid::API::ImplicitFunctionParserFactory::Instance()
        .subscribe<MockImplicitFunctionParserA>(
            "MockA1ImplicitFunctionParser"); // No warnings if used.
    Mantid::API::ImplicitFunctionParserFactory::Instance()
        .subscribe<testing::NiceMock<MockImplicitFunctionParserB>>(
            "MockB1ImplicitFunctionParser"); // Emit warnings if used.
  }

  void testCreateunwrappedSimple() {
    using Mantid::Geometry::MDImplicitFunction;
    Mantid::Geometry::MDImplicitFunction_sptr function(
        Mantid::API::ImplicitFunctionFactory::Instance().createUnwrapped(
            generateComplexXML()));

    TSM_ASSERT_EQUALS(
        "The correct implicit function type has not been generated",
        "MockImplicitFunctionA", function->getName());
  }

  void testCreateThrows() {
    TSM_ASSERT_THROWS(
        "Should have thrown exeption on use of create rather than "
        "createunwrapped.",
        Mantid::API::ImplicitFunctionFactory::Instance().create(""),
        const std::runtime_error &);
  }
};

#endif
