#include "MantidMDAlgorithms/CompositeImplicitFunction.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

namespace Mantid
{
    namespace MDAlgorithms
    {

        CompositeImplicitFunction::CompositeImplicitFunction()
        {	
        }

        CompositeImplicitFunction::~CompositeImplicitFunction()
        {
        }

        void CompositeImplicitFunction::addFunction(boost::shared_ptr<Mantid::API::ImplicitFunction> constituentFunction)
        {
            this->m_Functions.push_back(constituentFunction);
        }

        std::string CompositeImplicitFunction::getName() const
        {
            return CompositeImplicitFunction::functionName();
        }

        std::string CompositeImplicitFunction::toXMLString() const
        {
            using namespace Poco::XML;
            AutoPtr<Document> pDoc = new Document;
            AutoPtr<Element> functionElement = pDoc->createElement("Function");
            pDoc->appendChild(functionElement);
            AutoPtr<Element> typeElement = pDoc->createElement("Type");
            AutoPtr<Text> typeText = pDoc->createTextNode(this->getName());
            typeElement->appendChild(typeText);
            functionElement->appendChild(typeElement);
            AutoPtr<Element> parameterListElement = pDoc->createElement("ParameterList");
            functionElement->appendChild(parameterListElement);

            std::string functionXML;
            for(FunctionIterator it = m_Functions.begin();it!=m_Functions.end(); ++it)
            {
                functionXML += (*it)->toXMLString();
            }
            AutoPtr<Text> functionFormatText = pDoc->createTextNode("%s");
            functionElement->appendChild(functionFormatText);


            std::stringstream xmlstream;
            DOMWriter writer;
            writer.writeNode(xmlstream, pDoc);

            std::string formattedXMLString = boost::str(boost::format(xmlstream.str().c_str()) % functionXML.c_str());
            return formattedXMLString;
        }

        int CompositeImplicitFunction::getNFunctions() const
        {
            return this->m_Functions.size();
        }


        bool CompositeImplicitFunction::evaluate(const API::Point3D*  pPoint3D) const
        {
            bool evalResult = false;
            std::vector<boost::shared_ptr<Mantid::API::ImplicitFunction> >::const_iterator it;
            for(it = this->m_Functions.begin(); it != this->m_Functions.end(); ++it)
            {
                evalResult = (*it)->evaluate(pPoint3D);
                if(!evalResult)
                {
                    break;
                }
            }
            return evalResult;
        }

        bool CompositeImplicitFunction::operator==(const CompositeImplicitFunction &other) const
        {

            bool evalResult = false;
            if(other.getNFunctions() != this->getNFunctions() )
            {
                evalResult = false;
            }
            else if(other.getNFunctions() == 0)
            {
                evalResult = false;
            }
            else
            {
                for(size_t i = 0; i < this->m_Functions.size(); i++)
                {
                    evalResult = false; //TODO call equals operations on nested implicit functions.
                    if(!evalResult)
                    {
                        break;
                    }
                }
            }
            return evalResult;
        }

        //TODO. retire this function, call evaluate instead!
        std::vector<boost::shared_ptr<Mantid::API::ImplicitFunction> > CompositeImplicitFunction::getFunctions() const
        {
          return this->m_Functions;
        }
            
        bool CompositeImplicitFunction::operator!=(const CompositeImplicitFunction &other) const
        {
            return !(*this == other);
        }


    }
}
