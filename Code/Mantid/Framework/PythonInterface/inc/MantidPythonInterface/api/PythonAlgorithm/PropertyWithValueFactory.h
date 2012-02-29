#ifndef MANTID_PYTHONINTERFACE_PROEPRTYWITHVALUEFACTORY_H_
#define MANTID_PYTHONINTERFACE_PROEPRTYWITHVALUEFACTORY_H_
/**
    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/System.h"
#include <string>
#include <boost/python/object.hpp>

namespace Mantid
{
  //---------------------------------------------------------------------------
  // Forward declarations
  //---------------------------------------------------------------------------
  namespace Kernel
  {
    class Property;
  }

  namespace PythonInterface
  {
    /**
     * Defines a static factory class that creates PropertyWithValue
     * instances from python objects.
     */
    class PropertyWithValueFactory
    {
      /// Creates a property from the given value and direction
      static Kernel::Property * createProperty(const std::string & name, const boost::python::object & defaultValue, 
                                               const unsigned int direction);
      /// Creates a property from the value, validator and direction
      static Kernel::Property * createProperty(const std::string & name, const boost::python::object & defaultValue, 
                                               const boost::python::object & validator, const unsigned int direction);
    };
  }
}

#endif //MANTID_PYTHONINTERFACE_PROEPRTYWITHVALUEFACTORY_H_