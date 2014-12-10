#ifndef INVALID_PARAMETER_PARSER_H_
#define INVALID_PARAMETER_PARSER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>

#include "MantidKernel/System.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidMDAlgorithms/InvalidParameter.h"
#include "MantidAPI/ImplicitFunctionParameterParser.h"

namespace Mantid
{
    namespace MDAlgorithms
    {
        /** XML Parser for invalid parameter types

        @author Owen Arnold, Tessella plc
        @date 01/10/2010

        Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

        File change history is stored at: <https://github.com/mantidproject/mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
        */

        class DLLExport InvalidParameterParser : public Mantid::API::ImplicitFunctionParameterParser
        {
        public:
            InvalidParameterParser();
            Mantid::API::ImplicitFunctionParameter* createParameter(Poco::XML::Element* parameterElement);
            void setSuccessorParser(Mantid::API::ImplicitFunctionParameterParser* paramParser);
        protected:

            ImplicitFunctionParameterParser::SuccessorType m_successor;
            InvalidParameter* parseInvalidParameter(std::string value);
        };
    }
}

#endif
