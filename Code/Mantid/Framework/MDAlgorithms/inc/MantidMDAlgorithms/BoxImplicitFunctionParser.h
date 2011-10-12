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
            /// Default constructor
            BoxImplicitFunctionParser();

            /**
             * Create a function builder from a text-based representation.
             * @param functionElement text representation of function
             * @return the builder for the requested function
             */
            Mantid::API::ImplicitFunctionBuilder* createFunctionBuilder(Poco::XML::Element* functionElement);

            /**
             * Set the successor translation class.
             * @param parser the successor translator
             */
            void setSuccessorParser(Mantid::API::ImplicitFunctionParser* parser);

            /**
             * Set the parameter translation class.
             * @param parser the parameter translator
             */
            void setParameterParser(Mantid::API::ImplicitFunctionParameterParser* parser);

            /**
             * Create a box function builder from a text-based representation.
             * @param functionElement text representation of builder
             * @return a concrete box builder
             */
            BoxFunctionBuilder* parseBoxFunction(Poco::XML::Element* functionElement);

            /// Default destructor
            ~BoxImplicitFunctionParser();
        };
    }
}


#endif
