#ifndef MANTID_ALGORITHMS_BOX_IMPLICITFUNCTION_PARSER_H_
#define MANTID_ALGORITHMS_BOX_IMPLICITFUNCTION_PARSER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>
#include "MantidMDAlgorithms/BoxFunctionBuilder.h"
#include "MantidAPI/ImplicitFunctionParser.h"
#include "MantidAPI/ImplicitFunctionParameterParser.h"

namespace Mantid
{
    namespace MDDataObjects
    {
        class point3D;
    }
    namespace MDAlgorithms
    {
        /**
        This class to parse plane type functions and generate the associated builders.

        @author Owen Arnold, Tessella plc
        @date 09/12/2010

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

        class DLLExport BoxImplicitFunctionParser : public Mantid::API::ImplicitFunctionParser
        {

        public:
            BoxImplicitFunctionParser();

            Mantid::API::ImplicitFunctionBuilder* createFunctionBuilder(Poco::XML::Element* functionElement);

            void setSuccessorParser(Mantid::API::ImplicitFunctionParser* parser);

            void setParameterParser(Mantid::API::ImplicitFunctionParameterParser* parser);

            BoxFunctionBuilder* parseBoxFunction(Poco::XML::Element* functionElement);

            ~BoxImplicitFunctionParser();
        };
    }
}


#endif
