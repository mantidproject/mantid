#include "MantidPythonInterface/api/PythonAlgorithm/AlgorithmWrapper.h"
#include "MantidPythonInterface/kernel/Environment/WrapperHelpers.h"
#include <boost/python/class.hpp>

using namespace boost::python;

//-----------------------------------------------------------------------------
// AlgorithmWrapper definition
//-----------------------------------------------------------------------------
namespace Mantid
{
  namespace PythonInterface
  {

    /**
     * Returns the name of the algorithm. If not overridden
     * it returns the class name
     */
    const std::string AlgorithmWrapper::name() const
    {
      static const char * method = "name";
      std::string name;
      if( WrapperHelpers::typeHasAttribute(*this, method) )
      {
        name = call<std::string>(get_override(method).ptr()); // Avoid a warning with just calling return fn() which docs say you can do.
      }
      else
      {
        name = this->defaultName();
      }
      return name;
    }

    /**
     * Returns the base class version of name
     */
    const std::string AlgorithmWrapper::defaultName() const
    {
      // Use the class name
      PyObject *self = boost::python::detail::wrapper_base_::get_owner(*this);
      return std::string(self->ob_type->tp_name);
    }

    /**
     * Returns the version of the algorithm. If not overridden
     * it returns 1
     */
    int AlgorithmWrapper::version() const
    {
      static const char * method = "version";
      int version(1);
      if( WrapperHelpers::typeHasAttribute(*this, method) )
      {
        version = call<int>(get_override(method).ptr()); // Avoid a warning with just calling return fn() which docs say you can do.
      }
      else
      {
        version = this->defaultVersion();
      }
      return version;
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
      static const char * method = "category";
      std::string cat("PythonAlgorithms");
      if (  WrapperHelpers::typeHasAttribute(*this, method) )
      {
        cat = boost::python::call<std::string>(this->get_override(method).ptr());
      }
      return cat;
    }

    /**
     * Private init for this algorithm. Expected to be
     * overridden in the subclass by a function named PyInit
     */
    void AlgorithmWrapper::init()
    {
      if( boost::python::override fn = this->get_override("PyInit") )
      {
        fn();
      }
      else
      {
        std::ostringstream os;
        os << "Python algorithm '" << this->name() << "' does not define the PyInit function, cannot initialize.";
        throw std::runtime_error(os.str());
      }
    }

    /**
     * Private exec for this algorithm. Expected to be
     * overridden in the subclass by a function named PyExec
     */
    void AlgorithmWrapper::exec()
    {
      if( boost::python::override fn = this->get_override("PyExec") )
      {
        fn();
      }
      else
      {
        std::ostringstream os;
        os << "Python algorithm '" << this->name() << "' does not define the PyExec function, cannot execute.";
        throw std::runtime_error(os.str());
      }
    }

  }
}
