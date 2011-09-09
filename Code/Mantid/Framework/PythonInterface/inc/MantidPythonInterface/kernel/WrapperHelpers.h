#ifndef MANTID_PYTHONINTERFACE_WRAPPERHELPERS_H_
#define MANTID_PYTHONINTERFACE_WRAPPERHELPERS_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/System.h"
#include <boost/python/wrapper.hpp>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace WrapperHelpers
    {
      /**
      This namespace contains helper functions for classes that are overridden in Python

      @author Martyn Gigg, Tessella plc

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

      /** Checks whether the given object's type dictionary contains the named attribute.
       * Usually boost::python::get_override is used for this check but if the 
       * object's overridden function is declared on a superclass of the wrapped
       * class then get_override always returns true, regardless of whether the
       * method has been overridden in Python.
       *
       * An example is the algorithm hierachy. We export the IAlgorithm interface
       * with the name method attach to it. If a class in Python does not
       * override the name method then get_override still claims that the
       * override exists because it has found the IAlgorithm one
       */
      bool DLLExport typeHasAttribute(PyObject * obj, const char * attr);

      /// An overload for the above taking a wrapper reference
      bool DLLExport typeHasAttribute(const boost::python::detail::wrapper_base & wrapper, const char * attr);
    }
  }
}


#endif //MANTID_PYTHONINTERFACE_WRAPPERHELPERS_H_