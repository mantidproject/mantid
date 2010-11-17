#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/File.h"
#include "Poco/Path.h"
#include "MantidAPI/ImplicitFunctionParserFactory.h"

namespace Mantid
{
  namespace API
  {
    ImplicitFunctionParserFactoryImpl::ImplicitFunctionParserFactoryImpl()
    {
    }

    ImplicitFunctionParserFactoryImpl::~ImplicitFunctionParserFactoryImpl()
    {
    }

    boost::shared_ptr<ImplicitFunctionParser> ImplicitFunctionParserFactoryImpl::create(const std::string& xmlString) const
    {
      throw std::runtime_error("Use of create in this context is forbidden. Use createUnwrappedInstead.");
    }

    std::auto_ptr<ImplicitFunctionParser> ImplicitFunctionParserFactoryImpl::createImplicitFunctionParserFromXML(const std::string& configXML) const
    {
      using namespace Poco::XML;
      DOMParser pParser;
      Document* pDoc = pParser.parseString(configXML);
      Element* pRootElem = pDoc->documentElement();

      if(pRootElem->localName() != "Factories")
      {
        throw std::runtime_error("Root node must be a Fatories element");
      }
      NodeList* functionParserNodes = pRootElem->getChildElement("FunctionParserFactoryList")->childNodes();

      ImplicitFunctionParser* functionParser = NULL;
      for(size_t i = 0; i < functionParserNodes->length(); i++)
      {
        std::string functionParserName = functionParserNodes->item(i)->innerText();
        ImplicitFunctionParser* childParamParser = this->createUnwrapped(functionParserName);
        if(functionParser != NULL)
        {
          functionParser->setSuccessorParser(childParamParser);
        }
        else
        {
          functionParser = childParamParser;
        }
      }

      //Attach parameter parsers
      std::auto_ptr<ImplicitFunctionParameterParser> paramParser = Mantid::API::ImplicitFunctionParameterParserFactory::Instance().createImplicitFunctionParameterParserFromXML(configXML);

      functionParser->setParameterParser(paramParser.release());
      return std::auto_ptr<ImplicitFunctionParser>(functionParser);
    }
  }
}
