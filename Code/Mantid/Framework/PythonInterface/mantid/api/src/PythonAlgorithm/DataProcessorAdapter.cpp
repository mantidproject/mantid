#include "MantidPythonInterface/api/PythonAlgorithm/DataProcessorAdapter.h"

//-----------------------------------------------------------------------------
// AlgorithmAdapter definition
//-----------------------------------------------------------------------------
namespace Mantid
{
  namespace PythonInterface
  {

    /**
     * Construct the "wrapper" and stores the reference to the PyObject
     * @param self A reference to the calling Python object
     */
    DataProcessorAdapter::DataProcessorAdapter(PyObject* self)
      : SuperClass(self)
    {
    }

  }
}
