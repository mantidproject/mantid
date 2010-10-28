#ifndef FUNCTION_ABSTRACT_PARSER_H_
#define FUNCTION_ABSTRACT_PARSER_H_

/* Used to register classes into the factory. creates a global object in an
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's
 * subscribe method.
 */
#define DECLARE_IMPLICIT_FUNCTION_PARSER(classname) \
    namespace { \
    Mantid::Kernel::RegistrationHelper register_alg_##classname( \
    ((Mantid::API::ImplicitFunctionParserFactory::Instance().subscribe<classname>(#classname##Parser)) \
    , 0)); \
    }

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include <memory>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/File.h"
#include "Poco/Path.h"

#include "MantidKernel/System.h"
#include "MantidKernel/ArrayProperty.h"
#include "ImplicitFunctionBuilder.h"
#include "ImplicitFunctionParameterParser.h"



namespace Mantid
{
    namespace API
    {
        /** 
        XML Parser for function types. See chain of reponsibility pattern.

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

        class DLLExport ImplicitFunctionParser
        {
        protected:
            std::auto_ptr<ImplicitFunctionParameterParser> m_paramParserRoot; //Chain of responsibility 
            std::auto_ptr<ImplicitFunctionParser> m_successor;

            
            ImplicitFunctionParameter* parseParameter(Poco::XML::Element* pRoot)
            {
                return m_paramParserRoot->createParameter(pRoot);
            }

            void checkSuccessorExists()
            {
                if(0 == m_successor.get())
                {
                    std::string message = "There is no successor function parser"; 
                    throw std::runtime_error(message);
                }
            }

        public:
            ImplicitFunctionParser(ImplicitFunctionParameterParser* parameterParser) : m_paramParserRoot(std::auto_ptr<ImplicitFunctionParameterParser>(parameterParser))
            {
            }

            virtual ImplicitFunctionBuilder* createFunctionBuilder(Poco::XML::Element* functionElement) = 0;
            virtual void setSuccessorParser(ImplicitFunctionParser* parser) = 0;
            virtual ~ImplicitFunctionParser() = 0 {;}

        };

    }
}

#endif