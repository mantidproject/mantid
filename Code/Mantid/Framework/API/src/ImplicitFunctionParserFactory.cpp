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

    ImplicitFunctionParser* ImplicitFunctionParserFactoryImpl::createImplicitFunctionParserFromXML(Poco::XML::Element* functionElement) const
    {
      std::string name = functionElement->localName();
      if(name != "Function")
      {
        throw std::runtime_error("Root node must be a Funtion element. Unable to determine parsers.");
      }

      Poco::XML::Element* typeElement = functionElement->getChildElement("Type");
      std::string functionParserName = typeElement->innerText() + "Parser";
      ImplicitFunctionParser* functionParser = this->createUnwrapped(functionParserName);

      Poco::XML::Element* parametersElement = functionElement->getChildElement("ParameterList");

      //Get the parameter parser for the current function parser and append that to the function parser.
      ImplicitFunctionParameterParser* paramParser = Mantid::API::ImplicitFunctionParameterParserFactory::Instance().createImplicitFunctionParameterParserFromXML(parametersElement);
      functionParser->setParameterParser(paramParser);

      Poco::XML::NodeList* childFunctions = functionElement->getElementsByTagName("Function");
      ImplicitFunctionParser* childParser;
      for(unsigned long i = 0; i < childFunctions->length(); i++)
      {
        Poco::XML::Node* childFunctionNode = childFunctions->item(i);

        //Recursive call to handle nested parameters.
        Poco::XML::Element* childFunctionElement =( Poco::XML::Element*)childFunctionNode;
        ImplicitFunctionParser* tempParser = createImplicitFunctionParserFromXML(childFunctionElement);
        if(i==0)
        {
         childParser = tempParser;
         //Add the first child function parser to the parent (composite) directly.
         functionParser->setSuccessorParser(childParser);
        }
        else
        {
          //Add all other function parsers are added as successors to those before them in the loop.
          childParser->setSuccessorParser(tempParser);
          childParser = tempParser;
        }
      }

      return functionParser;
    }

    ImplicitFunctionParser* ImplicitFunctionParserFactoryImpl::createImplicitFunctionParserFromXML(const std::string& functionXML) const
    {
      using namespace Poco::XML;
      DOMParser pParser;
      Document* pDoc = pParser.parseString(functionXML);
      Element* pRootElem = pDoc->documentElement();

      return createImplicitFunctionParserFromXML(pRootElem);
    }

  }
}
