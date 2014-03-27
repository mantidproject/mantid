#ifndef MANTID_PYTHON_INTERFACE_DATAPROCESSORADAPTER_H
#define MANTID_PYTHON_INTERFACE_DATAPROCESSORADAPTER_H
/**
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/api/PythonAlgorithm/AlgorithmAdapter.h"
#include "MantidAPI/DataProcessorAlgorithm.h"

namespace Mantid
{
  namespace PythonInterface
  {
    /**
     * Provides a layer class for boost::python to allow C++ virtual functions
     * to be overridden in a Python object that is derived an DataProcessorAlgorithm.
     *
     * It also provides access to the protected methods on DataProcessorAlgorithm
     * from the type exported to Python
     */
    class DataProcessorAdapter : public AlgorithmAdapter<API::DataProcessorAlgorithm>
    {
      typedef AlgorithmAdapter<API::DataProcessorAlgorithm> SuperClass;

    public:
      /// A constructor that looks like a Python __init__ method
      DataProcessorAdapter(PyObject* self);

      // Public access for protected base methods for boost.python to call
      using API::DataProcessorAlgorithm::setLoadAlg;

    private:
      /// The PyObject must be supplied to construct the object
      DISABLE_DEFAULT_CONSTRUCT(DataProcessorAdapter);
      DISABLE_COPY_AND_ASSIGN(DataProcessorAdapter);
    };

  }
}

#endif // MANTID_PYTHON_INTERFACE_DATAPROCESSORADAPTER_H
