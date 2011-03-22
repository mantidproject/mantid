#ifndef IIMPLICITFUNCTION_H_
#define IIMPLICITFUNCTION_H_

/* Used to register classes into the factory. creates a global object in an
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's
 * subscribe method.
 */
#define DECLARE_IMPLICIT_FUNCTION(classname) \
    namespace { \
    Mantid::Kernel::RegistrationHelper register_alg_##classname( \
    ((Mantid::API::ImplicitFunctionFactory::Instance().subscribe<classname>(#classname)) \
    , 0)); \
    }

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/AutoPtr.h> 
#include <Poco/DOM/DOMWriter.h>
#include <Poco/XML/XMLWriter.h>
#include <sstream>
#include <vector>
#include "MantidKernel/System.h"
#include "Point3D.h"

namespace Mantid
{

    namespace API
    {
  

        /** A base class for absorption correction algorithms.

        This class represents the abstract implicit function type used for communicating and implementing an operation against 
        an MDWorkspace.

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

        class DLLExport ImplicitFunction
        {
        public:
            virtual bool evaluate(const Point3D* pPoint3D) const = 0;
            virtual std::string getName() const = 0; 
            virtual std::string toXMLString() const = 0;
            virtual ~ImplicitFunction()
            {
            }

        };
    }
}

#endif
