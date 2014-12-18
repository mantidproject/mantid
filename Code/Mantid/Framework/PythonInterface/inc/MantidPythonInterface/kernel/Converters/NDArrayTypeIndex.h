#ifndef MANTID_PYTHONINTERFACE_NDARRAYTYPEINDEX_H_
#define MANTID_PYTHONINTERFACE_NDARRAYTYPEINDEX_H_
/**
    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

namespace Mantid
{
  namespace PythonInterface
  {
    namespace Converters
    {

      /**
       * Defines a mapping between C++ type given by
       * the template parameter and numpy type enum
       * NPY_TYPES.
       *
       * There is no general definition, only specialized
       * versions are defined. Each specialization should
       * contain a static const NPY_TYPES definition giving
       * the result of the mapping
       */
      template<typename T>
      struct DLLExport NDArrayTypeIndex
      {
        static int typenum;
      };

    }
  }
}

#endif /* MANTID_PYTHONINTERFACE_NDARRAYTYPEINDEX_H_*/
