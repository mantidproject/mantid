#ifndef IPARAMETER_H_
#define IPARAMETER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/Text.h"
#include "Poco/DOM/AutoPtr.h" 
#include "Poco/DOM/DOMWriter.h"
#include "Poco/XML/XMLWriter.h"
#include <sstream>
#include <vector>
#include "MantidKernel/System.h"
#include "boost/smart_ptr/shared_ptr.hpp"

namespace Mantid
{
    namespace API
    {
        /** A base class for absorption correction algorithms.

        IProperty abstract property type for use with IImplicitFunctions.

        @author Owen Arnold, Tessella plc
        @date 01/10/2010

        Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

        This file is part of Mantid.

        Mantid is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 3 of the License, or
        (at your option) any later version.

        Mantid is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.

        File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
        */

        class DLLExport ImplicitFunctionParameter
        {

        public:
            virtual std::string getName() const = 0;

            virtual bool isValid() const = 0;

            virtual std::string toXMLString() const = 0;


            std::auto_ptr<ImplicitFunctionParameter> clone() const
            { 
                return std::auto_ptr<ImplicitFunctionParameter>( this->cloneImp() ); 
            }

            virtual ~ImplicitFunctionParameter() = 0
            {
            };

        protected:

            virtual ImplicitFunctionParameter* cloneImp() const = 0;

            bool m_isValid;

            std::string parameterXMLTemplate(std::string valueXMLtext) const
            {
                using namespace Poco::XML;

                using namespace Poco::XML;
                AutoPtr<Document> pDoc = new Document;
                Element* paramElement = pDoc->createElement("Parameter");
                
                pDoc->appendChild(paramElement);
                Element* typeElement = pDoc->createElement("Type");
                Text* typeText = pDoc->createTextNode(this->getName());
                typeElement->appendChild(typeText);
                paramElement->appendChild(typeElement);

                Element* valueElement = pDoc->createElement("Value");
                Text* valueText = pDoc->createTextNode(valueXMLtext);
                valueElement->appendChild(valueText);
                paramElement->appendChild(valueElement);
                
                std::stringstream xmlstream;

                DOMWriter writer;
                writer.writeNode(xmlstream, pDoc);
                return xmlstream.str();
            }


        };
    }
}

#endif