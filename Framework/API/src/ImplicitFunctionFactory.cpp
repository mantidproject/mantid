// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/ImplicitFunctionFactory.h"
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <boost/scoped_ptr.hpp>

namespace Mantid::API {

Mantid::Geometry::MDImplicitFunction_sptr ImplicitFunctionFactoryImpl::create(const std::string &className) const {
  UNUSED_ARG(className);
  throw std::runtime_error("Use of create in this context is forbidden. Use "
                           "createUnwrappedInstead.");
}

Mantid::Geometry::MDImplicitFunction *
ImplicitFunctionFactoryImpl::createUnwrapped(Poco::XML::Element *processXML) const {

  ImplicitFunctionParser *funcParser =
      Mantid::API::ImplicitFunctionParserFactory::Instance().createImplicitFunctionParserFromXML(processXML);

  boost::scoped_ptr<ImplicitFunctionBuilder> functionBuilder(funcParser->createFunctionBuilder(processXML));
  return functionBuilder->create();
}

Mantid::Geometry::MDImplicitFunction *
ImplicitFunctionFactoryImpl::createUnwrapped(const std::string &processXML) const {
  using namespace Poco::XML;
  DOMParser pParser;
  Poco::AutoPtr<Document> pDoc = pParser.parseString(processXML);
  Element *pInstructionsXML = pDoc->documentElement();

  boost::scoped_ptr<ImplicitFunctionParser> funcParser(
      ImplicitFunctionParserFactory::Instance().createImplicitFunctionParserFromXML(processXML));

  boost::scoped_ptr<ImplicitFunctionBuilder> functionBuilder(funcParser->createFunctionBuilder(pInstructionsXML));
  return functionBuilder->create();
}
} // namespace Mantid::API
