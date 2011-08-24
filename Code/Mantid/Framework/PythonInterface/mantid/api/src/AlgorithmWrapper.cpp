#include "MantidPythonInterface/api/AlgorithmWrapper.h"
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
     * Destructor
     */
    AlgorithmWrapper::~AlgorithmWrapper()
    {
    }

    /**
     * Returns the name of the algorithm. If not overridden
     * it returns the class name
     */
    const std::string AlgorithmWrapper::name() const
    {
      std::string name("");
      if( boost::python::override fn = this->get_override("name_") )
      {
        name = boost::python::call<std::string>(fn.ptr()); // Avoid a warning with just calling return fn() which docs say you can do.
      }
      else
      {
        // Use the class name
        PyObject *self = boost::python::detail::wrapper_base_::get_owner(*this);
        name = std::string(self->ob_type->tp_name);
      }
      return name;
    }

    /**
     * Returns the version of the algorithm. If not overridden
     * it returns 1
     */
    int AlgorithmWrapper::version() const
    {
      int version(1);
      if( boost::python::override fn = this->get_override("version_") )
      {
        version = boost::python::call<int>(fn.ptr());
      }
      return version;
    }

    /**
     * Returns the category of the algorithm. If not overridden
     * it returns "PythonAlgorithm"
     */
    const std::string AlgorithmWrapper::category() const
    {
      std::string cat("PythonAlgorithm");
      if ( boost::python::override fn = this->get_override("category_") )
      {
        cat = boost::python::call<std::string>(fn.ptr());
      }
      return cat;
    }

    /**
     * Private init for this algorithm. Expected to be
     * overridden in the subclass by a function named
     * init_
     */
    void AlgorithmWrapper::init()
    {
      if( boost::python::override fn = this->get_override("init_") )
      {
        fn();
      }
      else
      {
        std::ostringstream os;
        os << "Python algorithm '" << this->name() << "' does not define the init_ function, cannot initialize.";
        throw std::runtime_error(os.str());
      }
    }

    /**
     * Private exec for this algorithm. Expected to be
     * overridden in the subclass by a function named
     * "exec_"
     */
    void AlgorithmWrapper::exec()
    {
      if( boost::python::override fn = this->get_override("init_") )
      {
        fn();
      }
      else
      {
        std::ostringstream os;
        os << "Python algorithm '" << this->name() << "' does not define the exec_ function, cannot execute.";
        throw std::runtime_error(os.str());
      }
    }

  }
}
