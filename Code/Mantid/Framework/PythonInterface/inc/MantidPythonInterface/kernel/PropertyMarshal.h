#ifndef MANTID_PYTHONINTERFACE_PROPERTYMARSHAL_H_
#define MANTID_PYTHONINTERFACE_PROPERTYMARSHAL_H_

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "MantidKernel/System.h"
#include <boost/python/object.hpp>
#include <string>

namespace Mantid
{
  //---------------------------------------------------------------------------
  // Forward declaration
  //---------------------------------------------------------------------------
  namespace Kernel
  {
    class IPropertyManager;
  }

  namespace PythonInterface
  {

    /**
     * Python is dynamically typed so that the type of a variable is not
     * known until run time. The [set,get]Property methods on an algorithm
     * expect the value passed/returned to match that of the declared property
     * type, i.e. an integer property must use alg.setProperty(name, int).
     *
     * Boost.Python is able to declare several 'overloaded' functions of the
     * same name but if the number of arguments are the same and only the
     * types differ it is not able to figure out the correct function to
     * call. It instead chooses the last from the exported list that contains
     * a type that the value can be converted to. This then raises an exception
     * in Mantid.
     *
     * The helpers declared here deal with calling the correct function depending
     * on the type passed to it.

     * We will also need marshaling for these functions as we want be able to
     * pass numpy arrays to algorithms.
     */

    /**
     * A namespace for marshaling calls involving transferring property values in/out of an IPropertyManager.
     *
     * This allows us to have a single method that is called when a user runs, from Python, alg.setProperty
     * or property.value. For the value return it attempts to upcast the object to correct type
     *
     */
    namespace PropertyMarshal
    {
      /// This static function allows a call to a method on an IPropertyManager object
      DLLExport void setProperty(Kernel::IPropertyManager &self, const std::string & name,
                                 boost::python::object value);
      /// Upcast an item from a DataItem to the most exported known type
      DLLExport void upcastFromDataItem(boost::python::object value);
      /// Converts the value of a property to the most appropriate type, i.e. the most derived exported interface
      DLLExport boost::python::object value(boost::python::object self);
    };

  }
}

#endif /* MANTID_PYTHONINTERFACE_PROPERTYMARSHAL_H_ */
