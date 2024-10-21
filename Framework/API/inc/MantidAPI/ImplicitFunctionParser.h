// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/* Used to register classes into the factory. creates a global object in an
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's
 * subscribe method.
 */
// #define Parser Parser
#define DECLARE_IMPLICIT_FUNCTION_PARSER(classname)                                                                    \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper register_alg_##classname(                                                         \
      ((Mantid::API::ImplicitFunctionParserFactory::Instance().subscribe<classname>(#classname)), 0));                 \
  }

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <memory> //HACK
#include <vector>

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/File.h>
#include <Poco/Path.h>

#include "ImplicitFunctionBuilder.h"
#include "ImplicitFunctionParameterParser.h"
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace API {
/**
 XML Parser for function types. See chain of reponsibility pattern.

 @author Owen Arnold, Tessella plc
 @date 01/10/2010
 */

class MANTID_API_DLL ImplicitFunctionParser {
public:
  /// Successor type. Unique pointer with stack scoped deletion semantics.
  using SuccessorType = boost::interprocess::unique_ptr<ImplicitFunctionParser, DeleterPolicy<ImplicitFunctionParser>>;

protected:
  ImplicitFunctionParameterParser::SuccessorType m_paramParserRoot; // Chain of responsibility

  SuccessorType m_successor;

  ImplicitFunctionParameter *parseParameter(Poco::XML::Element *pRoot) {
    return m_paramParserRoot->createParameter(pRoot);
  }

  void checkSuccessorExists() {
    if (!m_successor) {
      std::string message = "There is no successor function parser. Is this an "
                            "empty composite function?";
      throw std::runtime_error(message);
    }
  }

public:
  ImplicitFunctionParser(ImplicitFunctionParameterParser *parameterParser) : m_paramParserRoot(parameterParser) {}

  virtual ImplicitFunctionBuilder *createFunctionBuilder(Poco::XML::Element *functionElement) = 0;
  virtual void setSuccessorParser(ImplicitFunctionParser *parser) = 0;
  virtual void setParameterParser(ImplicitFunctionParameterParser *parser) = 0;
  virtual ~ImplicitFunctionParser() = default;
};
} // namespace API
} // namespace Mantid
