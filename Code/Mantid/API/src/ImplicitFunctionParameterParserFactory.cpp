#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/File.h"
#include "Poco/Path.h"
#include "MantidAPI/ImplicitFunctionParameterParserFactory.h"

namespace Mantid
{
  namespace API
  {
    ImplicitFunctionParameterParserFactoryImpl::ImplicitFunctionParameterParserFactoryImpl()
    {
    }

    ImplicitFunctionParameterParserFactoryImpl::~ImplicitFunctionParameterParserFactoryImpl()
    {
    }

    boost::shared_ptr<ImplicitFunctionParameterParser> ImplicitFunctionParameterParserFactoryImpl::create(const std::string& xmlString) const
    {
      throw std::runtime_error("Use of create in this context is forbidden. Use createUnwrappedInstead.");
    }

    std::auto_ptr<ImplicitFunctionParameterParser> ImplicitFunctionParameterParserFactoryImpl::createImplicitFunctionParameterParserFromXML(const std::string& configXML) const
    {
      using namespace Poco::XML;
      DOMParser pParser;
      Document* pDoc = pParser.parseString(configXML);
      Element* pRootElem = pDoc->documentElement();
      if(pRootElem->localName() != "Factories")
      {
        throw std::runtime_error("Root node must be a Fatories element");
      }
      NodeList* paramParserNodes = pRootElem->getChildElement("ParameterParserFactoryList")->childNodes();

      ImplicitFunctionParameterParser* paramParser = NULL;
      for(size_t i = 0; i < paramParserNodes->length(); i++)
      {
        std::string paramParserName = paramParserNodes->item(i)->innerText();
        ImplicitFunctionParameterParser* childParamParser = this->createUnwrapped(paramParserName);
        if(paramParser != NULL)
        {
          paramParser->setSuccessorParser(childParamParser);
        }
        else
        {
          paramParser = childParamParser;
        }
      }
      return std::auto_ptr<ImplicitFunctionParameterParser>(paramParser);
    }

  }
}
