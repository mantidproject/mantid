#ifndef PARAMETER_PARSER_H_
#define PARAMETER_PARSER_H_

/* Used to register classes into the factory. creates a global object in an
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's
 * subscribe method.
 */
//#define Parser Parser
#define DECLARE_IMPLICIT_FUNCTION_PARAMETER_PARSER(classname) \
    namespace { \
    Mantid::Kernel::RegistrationHelper register_alg_##classname( \
    ((Mantid::API::ImplicitFunctionParameterParserFactory::Instance().subscribe<classname>(#classname)) \
    , 0)); \
    }

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>

#include <boost/shared_ptr.hpp>

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
#include "ImplicitFunctionParameter.h"

namespace Mantid
{
    namespace API
    {
        /** A base class for absorption correction algorithms.

        XML Parser for parameter types

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

        class DLLExport ImplicitFunctionParameterParser
        {
        public:
            virtual ImplicitFunctionParameter* createParameter(Poco::XML::Element* parameterElement) = 0;
            virtual void setSuccessorParser(ImplicitFunctionParameterParser* paramParser) = 0;
            virtual ~ImplicitFunctionParameterParser() = 0 {;}
        protected:
            std::auto_ptr<ImplicitFunctionParameterParser> m_successor;

        };
    }
}

#endif