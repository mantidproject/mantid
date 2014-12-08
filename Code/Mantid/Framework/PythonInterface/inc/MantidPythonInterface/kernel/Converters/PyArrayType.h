#ifndef MANTID_PYTHONINTERFACE_CONVERTERS_PYARRAYTYPE_H_
#define MANTID_PYTHONINTERFACE_CONVERTERS_PYARRAYTYPE_H_
/**
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
#include "MantidKernel/System.h"
#include <boost/python/detail/prefix.hpp>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace Converters
    {
      //
      // It is important that the numpy/arrayobject header
      // does not appear in any of our headers as it
      // contains some static definitions that cannot be
      // allowed to be defined in other translation units

      // Numpy array type
      DLLExport PyTypeObject * getNDArrayType();
    }
  }
}



#endif /* PYARRAYTYPE_H_ */
