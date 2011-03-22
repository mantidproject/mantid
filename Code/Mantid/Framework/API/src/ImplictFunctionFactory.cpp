
#include "MantidAPI/ImplicitFunctionFactory.h"
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <boost/scoped_ptr.hpp>

namespace Mantid
{
  namespace API
  {
    ImplicitFunctionFactoryImpl::ImplicitFunctionFactoryImpl()
    {
    }

    ImplicitFunctionFactoryImpl::~ImplicitFunctionFactoryImpl()
    {
    }

    boost::shared_ptr<ImplicitFunction> ImplicitFunctionFactoryImpl::create(const std::string& className) const
    {   
      throw std::runtime_error("Use of create in this context is forbidden. Use createUnwrappedInstead.");
    }

    ImplicitFunction* ImplicitFunctionFactoryImpl::createUnwrapped(Poco::XML::Element* processXML) const
    {

      ImplicitFunctionParser* funcParser = Mantid::API::ImplicitFunctionParserFactory::Instance().createImplicitFunctionParserFromXML(processXML);

      boost::scoped_ptr<ImplicitFunctionBuilder> functionBuilder(funcParser->createFunctionBuilder(processXML));
      return functionBuilder->create();
    }


    ImplicitFunction* ImplicitFunctionFactoryImpl::createUnwrapped(const std::string& processXML) const
    {
      using namespace Poco::XML;
      DOMParser pParser;
      Document* pDoc = pParser.parseString(processXML);
      Element* pInstructionsXML = pDoc->documentElement();

      ImplicitFunctionParser* funcParser = Mantid::API::ImplicitFunctionParserFactory::Instance().createImplicitFunctionParserFromXML(processXML);

      boost::scoped_ptr<ImplicitFunctionBuilder> functionBuilder(funcParser->createFunctionBuilder(pInstructionsXML));
      return functionBuilder->create();
    }
  }
}
