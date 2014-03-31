#include "MantidPythonInterface/api/PythonAlgorithm/AlgorithmAdapter.h"
#include "MantidPythonInterface/kernel/Registry/PropertyWithValueFactory.h"
#include "MantidPythonInterface/kernel/Environment/WrapperHelpers.h"
#include "MantidPythonInterface/kernel/Environment/CallMethod.h"
#include "MantidPythonInterface/kernel/Environment/Threading.h"
#include "MantidAPI/DataProcessorAlgorithm.h"

#include <boost/python/class.hpp>
#include <boost/python/dict.hpp>

//-----------------------------------------------------------------------------
// AlgorithmAdapter definition
//-----------------------------------------------------------------------------
namespace Mantid
{
  namespace PythonInterface
  {
    using namespace boost::python;
    using Environment::CallMethod0;

    /**
     * Construct the "wrapper" and stores the reference to the PyObject
     * @param self A reference to the calling Python object
     */
    template<typename BaseAlgorithm>
    AlgorithmAdapter<BaseAlgorithm>::AlgorithmAdapter(PyObject* self)
      : BaseAlgorithm(), m_self(self), m_isRunningObj(NULL)
    {
      // Cache the isRunning call to save the lookup each time it is called
      // as it is most likely called in a loop

      // If the derived class type has isRunning then use that.
      // A standard PyObject_HasAttr will check the whole inheritance
      // hierarchy and always return true because IAlgorithm::isRunning is present.
      // We just want to look at the Python class
      if(Environment::typeHasAttribute(self, "isRunning"))
        m_isRunningObj = PyObject_GetAttrString(self, "isRunning");
    }

    /**
     * Returns the name of the algorithm. This cannot be overridden in Python.
     */
    template<typename BaseAlgorithm>
    const std::string AlgorithmAdapter<BaseAlgorithm>::name() const
    {
      return std::string(getSelf()->ob_type->tp_name);
    }

    /**
     * Returns the version of the algorithm. If not overridden
     * it returns 1
     */
    template<typename BaseAlgorithm>
    int AlgorithmAdapter<BaseAlgorithm>::version() const
    {
      return CallMethod0<int>::dispatchWithDefaultReturn(getSelf(), "version", defaultVersion());
    }

    /**
     * Returns the default version of the algorithm. If not overridden
     * it returns 1
     */
    template<typename BaseAlgorithm>
    int AlgorithmAdapter<BaseAlgorithm>::defaultVersion() const
    {
      return 1;
    }

    /**
     * Returns the category of the algorithm. If not overridden
     * it returns "AlgorithmAdapter"
     */
    template<typename BaseAlgorithm>
    const std::string AlgorithmAdapter<BaseAlgorithm>::category() const
    {
      return CallMethod0<std::string>::dispatchWithDefaultReturn(getSelf(), "category", defaultCategory());
    }

    /**
     * A default category, chosen if there is no override
     * @returns A default category
     */
    template<typename BaseAlgorithm>
    std::string AlgorithmAdapter<BaseAlgorithm>::defaultCategory() const
    {
      return "PythonAlgorithms";
    }

    /**
     * @return True if the algorithm is considered to be running
     */
    template<typename BaseAlgorithm>
    bool AlgorithmAdapter<BaseAlgorithm>::isRunning() const
    {
      if(!m_isRunningObj)
      {
        return SuperClass::isRunning();
      }
      else
      {
        Environment::GlobalInterpreterLock gil;
        PyObject *result = PyObject_CallObject(m_isRunningObj, NULL);
        if(PyErr_Occurred()) Environment::throwRuntimeError(true);
        if(PyBool_Check(result)) return PyInt_AsLong(result);
        else throw std::runtime_error("AlgorithmAdapter.isRunning - Expected bool return type.");
      }
    }

    /**
     */
    template<typename BaseAlgorithm>
    void AlgorithmAdapter<BaseAlgorithm>::cancel()
    {
      // No real need for eye on performance here. Use standard methods
      if(Environment::typeHasAttribute(getSelf(), "cancel"))
      {
        Environment::GlobalInterpreterLock gil;
        CallMethod0<void>::dispatchWithException(getSelf(), "cancel");
      }
      else SuperClass::cancel();
    }

    /**
     * @copydoc Mantid::API::Algorithm::validateInputs
     */
    template<typename BaseAlgorithm>
    std::map<std::string, std::string> AlgorithmAdapter<BaseAlgorithm>::validateInputs()
    {
      // variables that are needed further down
      boost::python::dict resultDict;
      std::map<std::string, std::string> resultMap;

      // this is a modified version of CallMethod0::dispatchWithDefaultReturn
      Environment::GlobalInterpreterLock gil;
      if(Environment::typeHasAttribute(getSelf(), "validateInputs"))
      {
        try
        {
          resultDict = boost::python::call_method<boost::python::dict>(getSelf(),"validateInputs");

          if (!bool(resultDict))
            return resultMap;
        }
        catch(boost::python::error_already_set&)
        {
          Environment::throwRuntimeError();
        }
      }

      // convert to a map<string,string>
      boost::python::list keys = resultDict.keys();
      size_t numItems = boost::python::len(keys);
      for (size_t i = 0; i < numItems; ++i)
      {
        boost::python::object value = resultDict[keys[i]];
        if (value)
        {
          try
          {
            std::string key = boost::python::extract<std::string>(keys[i]);
            std::string value = boost::python::extract<std::string>(resultDict[keys[i]]);
            resultMap[key] = value;
          }
          catch(boost::python::error_already_set &)
          {
            this->getLogger().error() << "In validateInputs(self): Invalid type for key/value pair "
                                      << "detected in dict.\n"
                                      << "All keys and values must be strings\n";
          }
        }
      }
      return resultMap;
    }

    /**
     * Declare a preconstructed property.
     * @param prop :: A pointer to a property
     * @param doc :: An optional doc string
     */
    template<typename BaseAlgorithm>
    void AlgorithmAdapter<BaseAlgorithm>::declarePyAlgProperty(boost::python::object &self, Kernel::Property *prop, const std::string & doc)
    {
      BaseAlgorithm & caller = extract<BaseAlgorithm&>(self);
      // We need to clone the property so that python doesn't own the object that gets inserted
      // into the manager
      caller.declareProperty(prop->clone(), doc);
    }

    /**
     * Declare a property using the type of the defaultValue, a documentation string and validator
     * @param name :: The name of the new property
     * @param defaultValue :: A default value for the property. The type is mapped to a C++ type
     * @param validator :: A validator object
     * @param doc :: The documentation string
     * @param direction :: The direction of the property
     */
    template<typename BaseAlgorithm>
    void AlgorithmAdapter<BaseAlgorithm>::declarePyAlgProperty(boost::python::object &self, const std::string & name, const boost::python::object & defaultValue,
                                                               const boost::python::object & validator,
                                                               const std::string & doc, const int direction)
    {
      BaseAlgorithm & caller = extract<BaseAlgorithm&>(self);
      caller.declareProperty(Registry::PropertyWithValueFactory::create(name, defaultValue, validator, direction), doc);
    }

    /**
     * Declare a property using the type of the defaultValue and a documentation string
     * @param name :: The name of the new property
     * @param defaultValue :: A default value for the property. The type is mapped to a C++ type
     * @param doc :: The documentation string
     * @param direction :: The direction of the property
     */
    template<typename BaseAlgorithm>
    void AlgorithmAdapter<BaseAlgorithm>::declarePyAlgProperty(boost::python::object &self, const std::string & name, const boost::python::object & defaultValue,
                                                               const std::string & doc, const int direction)
    {
      BaseAlgorithm & caller = extract<BaseAlgorithm&>(self);
      caller.declareProperty(Registry::PropertyWithValueFactory::create(name, defaultValue, direction), doc);
    }

    /**
    * Declare a property using the type of the defaultValue
    * @param name :: The name of the new property
    * @param defaultValue :: A default value for the property. The type is mapped to a C++ type
    * @param direction :: The direction of the property
    */
    template<typename BaseAlgorithm>
    void AlgorithmAdapter<BaseAlgorithm>::declarePyAlgProperty(boost::python::object &self, const std::string & name, const boost::python::object & defaultValue,
                                                        const int direction)
    {
      declarePyAlgProperty(self, name, defaultValue, "", direction);
    }


    //---------------------------------------------------------------------------------------------
    // Private members
    //---------------------------------------------------------------------------------------------

    /**
     * Private init for this algorithm. Expected to be
     * overridden in the subclass by a function named PyInit
     */
    template<typename BaseAlgorithm>
    void AlgorithmAdapter<BaseAlgorithm>::init()
    {
      CallMethod0<void>::dispatchWithException(getSelf(), "PyInit");
    }

    /**
     * Private exec for this algorithm. Expected to be
     * overridden in the subclass by a function named PyExec
     */
    template<typename BaseAlgorithm>
    void AlgorithmAdapter<BaseAlgorithm>::exec()
    {
      CallMethod0<void>::dispatchWithException(getSelf(), "PyExec");
    }

    //-----------------------------------------------------------------------------------------------------------------------------
    // Concete instantiations (avoids definitions being all in the headers)
    //-----------------------------------------------------------------------------------------------------------------------------
    /// API::Algorithm as base
    template class AlgorithmAdapter<API::Algorithm>;
    /// API::DataProcesstor as base
    template class AlgorithmAdapter<API::DataProcessorAlgorithm>;

  }
}
