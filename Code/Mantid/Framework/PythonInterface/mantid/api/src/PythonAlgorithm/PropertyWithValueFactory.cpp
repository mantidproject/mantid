//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/api/PythonAlgorithm/PropertyWithValueFactory.h"
#include "MantidPythonInterface/kernel/Registry/SingleValueTypeHandler.h"
#include "MantidKernel/PropertyWithValue.h"
#include <cassert>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace
    {
      /// Lookup map type
      typedef std::map<PyTypeObject const *, Registry::PropertyValueHandler*> PyTypeIndex;
      /**
       * Initialize lookup map
       */
      void initTypeLookup(PyTypeIndex & index)
      {
        assert(index.empty());

        #define REGISTER_MAPPING(PyType, CppType)\
          index.insert(std::make_pair(&PyType, new Registry::SingleValueTypeHandler<CppType>()));

        // Map the Python types to the best match in C++
        REGISTER_MAPPING(PyFloat_Type, double);
        REGISTER_MAPPING(PyInt_Type, long);
        REGISTER_MAPPING(PyBool_Type, bool);
        REGISTER_MAPPING(PyString_Type, std::string);
      }

      /**
       * Returns a reference to the static lookup map
       */
      const PyTypeIndex & getTypeIndex()
      {
        static PyTypeIndex index;
        if( index.empty() ) initTypeLookup(index);
        return index;
      }
    }

    /**
     * Creates a PropertyWithValue<Type> instance from the given information.
     * The python type is mapped to a C type using the mapping defined by initPythonTypeMap()
     * @param name :: The name of the property
     * @param defaultValue :: A default value for this property.
     * @param validator :: A validator object
     * @param direction :: Specifies whether the property is Input, InOut or Output
     * @returns A pointer to a new Property object
     */
    Kernel::Property *
    PropertyWithValueFactory::create(const std::string & name , const boost::python::object & value, 
                                     const boost::python::object & validator, const unsigned int direction)
    {
      Registry::PropertyValueHandler *propHandle = lookup(value.ptr()->ob_type);
      return propHandle->create(name, value, validator, direction);
    }

    /**
     * Creates a PropertyWithValue<Type> instance from the given information.
     * The python type is mapped to a C type using the mapping defined by initPythonTypeMap()
     * @param name :: The name of the property
     * @param defaultValue :: A default value for this property.
     * @param direction :: Specifies whether the property is Input, InOut or Output
     * @returns A pointer to a new Property object
     */
    Kernel::Property *
    PropertyWithValueFactory::create(const std::string & name , const boost::python::object & value, 
                                     const unsigned int direction)
    {
      boost::python::object validator; // Default construction gives None object
      return create(name, value, validator, direction);
    }


    //-------------------------------------------------------------------------
    // Private methods
    //-------------------------------------------------------------------------
    /**
     * Return a handler that maps the python type to a C++ type
     * @param pythonType :: A pointer to a PyTypeObject that represents the type
     * @returns A pointer to handler that can be used to instantiate a property
     */ 
    Registry::PropertyValueHandler * PropertyWithValueFactory::lookup(PyTypeObject * const pythonType)
    {
      const PyTypeIndex & typeIndex = getTypeIndex();
      auto cit = typeIndex.find(pythonType);
      if( cit == typeIndex.end() )
      {
        std::ostringstream os;
        os << "Cannot create PropertyWithValue from Python type " << pythonType->tp_name << ". No converter registered in PropertyWithValueFactory.";
        throw std::invalid_argument(os.str());
      }
      return cit->second;
    }

  }
}
