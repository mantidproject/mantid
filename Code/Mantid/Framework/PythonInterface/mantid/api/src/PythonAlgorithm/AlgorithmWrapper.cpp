#include "MantidPythonInterface/api/PythonAlgorithm/AlgorithmWrapper.h"
#include "MantidPythonInterface/kernel/Environment/WrapperHelpers.h"
#include "MantidPythonInterface/kernel/Environment/CallMethod.h"

#include <boost/python/class.hpp>

//-----------------------------------------------------------------------------
// AlgorithmWrapper definition
//-----------------------------------------------------------------------------
namespace Mantid
{
  namespace PythonInterface
  {
    using namespace boost::python;
    using Environment::CallMethod_NoArg;

    /**
     * Construct the "wrapper" and stores the reference to the PyObject
     * * @param self A reference to the calling Python object
     */
    AlgorithmWrapper::AlgorithmWrapper(PyObject* self)
      : PythonAlgorithm(), m_self(self)
    {
    }

    /**
     * Returns the name of the algorithm. This cannot be overridden in Python.
     */
    const std::string AlgorithmWrapper::name() const
    {
      return std::string(getSelf()->ob_type->tp_name);
    }

    /**
     * Returns the version of the algorithm. If not overridden
     * it returns 1
     */
    int AlgorithmWrapper::version() const
    {
      return CallMethod_NoArg<int>::dispatchWithDefaultReturn(getSelf(), "version", defaultVersion());
    }

    int AlgorithmWrapper::defaultVersion() const
    {
      return 1;
    }

    /**
     * Returns the category of the algorithm. If not overridden
     * it returns "PythonAlgorithm"
     */
    const std::string AlgorithmWrapper::category() const
    {
      return CallMethod_NoArg<std::string>::dispatchWithDefaultReturn(getSelf(), "category", defaultCategory());
    }

    /**
     * A default category, chosen if there is no override
     * @returns A default category
     */
    std::string AlgorithmWrapper::defaultCategory() const
    {
      return "PythonAlgorithms";
    }

    /**
     * Private init for this algorithm. Expected to be
     * overridden in the subclass by a function named PyInit
     */
    void AlgorithmWrapper::init()
    {
      std::ostringstream os;
      os << "Python algorithm '" << this->name()
      << "' does not define the PyInit function, cannot initialize.";
      CallMethod_NoArg<void>::dispatchWithException(getSelf(), "PyInit", os.str().c_str());
    }

    /**
     * Private exec for this algorithm. Expected to be
     * overridden in the subclass by a function named PyExec
     */
    void AlgorithmWrapper::exec()
    {
      std::ostringstream os;
      os << "Python algorithm '" << this->name()
         << "' does not define the PyExec function, cannot execute.";
      CallMethod_NoArg<void>::dispatchWithException(getSelf(), "PyExec", os.str().c_str());
    }

  }
}
