//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Registry/SequenceTypeHandler.h"
#include "MantidPythonInterface/kernel/Converters/PySequenceToVector.h"
#include "MantidPythonInterface/kernel/Converters/NDArrayToVector.h"
#include "MantidKernel/IPropertyManager.h"

// See http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

#include <boost/python/extract.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace Registry
    {
      namespace
      {
        template <typename HeldType>
        struct StdVectorExtractor
        {
          static std::vector<HeldType> extract(const boost::python::object & value)
          {
            return boost::python::extract<std::vector<HeldType>>(value.ptr());
          }
        };
        template <>
        struct StdVectorExtractor<bool>
        {
          static std::vector<bool> extract(const boost::python::object &)
          {
            throw std::runtime_error("Unable to supported extracting std::vector<bool> from python object");
          }
        };

      }

      /**
       * Set function to handle Python -> C++ calls to a property manager and get the correct type
       * @param alg :: A pointer to an IPropertyManager
       * @param name :: The name of the property
       * @param value :: A boost python object that stores the container values
       */
      template<typename ContainerType>
      void SequenceTypeHandler<ContainerType>::set(Kernel::IPropertyManager* alg, const std::string &name,
                                                   const boost::python::object & value)
      {
        using boost::python::len;
        typedef typename ContainerType::value_type DestElementType;

        // Current workaround for things that still pass back wrapped vectors...
        if(boost::starts_with(value.ptr()->ob_type->tp_name, "std_vector"))
        {
          alg->setProperty(name, StdVectorExtractor<DestElementType>::extract(value));
        }
        // numpy arrays requires special handling to extract their types. Hand-off to a more appropriate handler
        else if( PyArray_Check(value.ptr()) )
        {
          alg->setProperty(name, Converters::NDArrayToVector<DestElementType>(value)());
        }
        else if( PySequence_Check(value.ptr()) )
        {
          alg->setProperty(name, Converters::PySequenceToVector<DestElementType>(value)());
        }
        else // assume it is a scalar and try to convert into a vector of length one
        {
          DestElementType scalar = boost::python::extract<DestElementType>(value.ptr());
          alg->setProperty(name, std::vector<DestElementType>(1, scalar));
        }
      }

      //-----------------------------------------------------------------------
      // Concrete instantiations
      //-----------------------------------------------------------------------
      ///@cond
      #define INSTANTIATE(ElementType)\
        template DLLExport struct SequenceTypeHandler<std::vector<ElementType> >;

      INSTANTIATE(int);
      INSTANTIATE(long);
      INSTANTIATE(long long);
      INSTANTIATE(unsigned int);
      INSTANTIATE(unsigned long);
      INSTANTIATE(unsigned long long);
      INSTANTIATE(double);
      INSTANTIATE(std::string);
      INSTANTIATE(bool);
      ///@endcond
    }
  }
}
