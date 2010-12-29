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

    ImplicitFunctionParameterParser* ImplicitFunctionParameterParserFactoryImpl::createImplicitFunctionParameterParserFromXML(Poco::XML::Element* parametersElement) const
    {
      if(parametersElement->localName() != "ParameterList")
      {
        throw std::runtime_error("Expected passed element to be ParameterList.");
      }
      Poco::XML::NodeList* parameters = parametersElement->getElementsByTagName("Parameter");
      ImplicitFunctionParameterParser* paramParser = NULL;
      ImplicitFunctionParameterParser* nextParser = NULL;
      for(unsigned long i = 0 ; i < parameters->length(); i++)
      {
        Poco::XML::Element* parameter = dynamic_cast<Poco::XML::Element*>(parameters->item(i));
        std::string paramParserName = parameter->getChildElement("Type")->innerText() + "Parser"; //Append parser to the name. Fixed convention
        ImplicitFunctionParameterParser* childParamParser = this->createUnwrapped(paramParserName);
        if(paramParser != NULL)
        {
          nextParser->setSuccessorParser(childParamParser);
        }
        else
        {
          paramParser = childParamParser;
        }
        nextParser = childParamParser;
      }
      return paramParser;
    }

  }
}
