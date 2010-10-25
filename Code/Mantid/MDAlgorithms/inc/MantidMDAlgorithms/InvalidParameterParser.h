#ifndef INVALID_PARAMETER_PARSER_H_
#define INVALID_PARAMETER_PARSER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>

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
#include "InvalidParameter.h"
#include "ParameterParser.h"

namespace Mantid
{
    namespace MDAlgorithms
    {
        /** A base class for absorption correction algorithms.

        XML Parser for invalid parameter types

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

        class DLLExport InvalidParameterParser : public ParameterParser
        {
        public:
            InvalidParameterParser();
            IParameter* createParameter(Poco::XML::Element* parameterElement);
            void setSuccessorParser(ParameterParser* paramParser);
        protected:
            std::auto_ptr<ParameterParser> m_successor;
            InvalidParameter* parseInvalidParameter(std::string value);
        };
    }
}

#endif