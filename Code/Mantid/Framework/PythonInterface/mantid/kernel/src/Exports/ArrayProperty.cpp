#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/NullValidator.h"

#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"
#include "MantidPythonInterface/kernel/Converters/NDArrayToVector.h"
#include "MantidPythonInterface/kernel/Converters/PySequenceToVector.h"

#include <boost/python/class.hpp>
#include <boost/python/list.hpp>
#include <boost/python/numeric.hpp>

#include <boost/python/make_constructor.hpp>
#include <boost/python/default_call_policies.hpp>

using Mantid::Kernel::ArrayProperty;
using Mantid::Kernel::PropertyWithValue;
using Mantid::Kernel::Direction;
using Mantid::Kernel::IValidator_sptr;
using Mantid::Kernel::NullValidator;
namespace Policies = Mantid::PythonInterface::Policies;
namespace Converters = Mantid::PythonInterface::Converters;
using namespace boost::python;

namespace
{
  /// return_value_policy for cloned numpy array
  typedef return_value_policy<Policies::VectorToNumpy<Converters::Clone> > return_cloned_numpy;

  #define EXPORT_ARRAY_PROP(type, prefix) \
      class_<ArrayProperty<type>, \
             bases<PropertyWithValue<std::vector<type> > >, \
             boost::noncopyable \
            >(#prefix"ArrayProperty", no_init) \
        .def(init<const std::string &, const unsigned int>((arg("name"), arg("direction") = Direction::Input), \
                                                         "Construct an ArrayProperty of type"#type)) \
         \
        .def(init<const std::string &, IValidator_sptr, \
                  const unsigned int>((arg("name"), arg("validator"), arg("direction") = Direction::Input), \
                                      "Construct an ArrayProperty of type"#type"with a validator")) \
         \
         .def(init<const std::string &, const std::string &, IValidator_sptr, \
                   const unsigned int>((arg("name"), arg("values"), arg("validator") = IValidator_sptr(new NullValidator), \
                                        arg("direction") = Direction::Input), \
                                       "Construct an ArrayProperty of type"#type"with a validator giving the values as a string")) \
         .def("__init__", make_constructor(&createArrayPropertyFromList<type>, default_call_policies(), \
                               (arg("name"), arg("values"), arg("validator") = IValidator_sptr(new NullValidator), \
                                arg("direction") = Direction::Input) \
                              ))\
        .def("__init__", make_constructor(&createArrayPropertyFromNDArray<type>, default_call_policies(), \
                              (arg("name"), arg("values"), arg("validator") = IValidator_sptr(new NullValidator), \
                               arg("direction") = Direction::Input) \
                             ))\
        .add_property("value", make_function(&ArrayProperty<type>::operator(), return_cloned_numpy())) \
      ;


    /**
     * Factory function to allow the initial values to be specified as a python list
     * @param name :: The name of the property
     * @param vec :: A boost python list of initial values
     * @param validator :: A validator object
     * @param direction :: A direction
     * @return
     */
    template<typename T>
    ArrayProperty<T> *
    createArrayPropertyFromList(const std::string &name, const boost::python::list & values,
                        IValidator_sptr validator, const unsigned int direction)
   {
     return new ArrayProperty<T>(name, Converters::PySequenceToVectorConverter<T>(values)(), validator, direction);
   }

   /**
     * Factory function to allow the initial values to be specified as a numpy array
     * @param name :: The name of the property
     * @param vec :: A boost python array of initial values
     * @param validator :: A validator object
     * @param direction :: A direction
     * @return
     */
    template<typename T>
    ArrayProperty<T> *
    createArrayPropertyFromNDArray(const std::string &name, const boost::python::numeric::array & values,
                        IValidator_sptr validator, const unsigned int direction)
   {
     return new ArrayProperty<T>(name, Converters::NDArrayToVectorConverter<T>(values)(), validator, direction);
   }

}

void export_ArrayProperty()
{
  EXPORT_ARRAY_PROP(double,Float);
  EXPORT_ARRAY_PROP(int,Int);
  EXPORT_ARRAY_PROP(size_t,UnsignedInt);
  EXPORT_ARRAY_PROP(std::string, String);
}

