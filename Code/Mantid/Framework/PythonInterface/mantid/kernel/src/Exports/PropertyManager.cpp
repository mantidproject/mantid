#ifdef _MSC_VER
  #pragma warning( disable: 4250 ) // Disable warning regarding inheritance via dominance, we have no way around it with the design
#endif

#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidPythonInterface/kernel/Registry/TypeRegistry.h"
#include "MantidPythonInterface/kernel/Registry/PropertyValueHandler.h"
#include "MantidPythonInterface/kernel/Registry/PropertyWithValueFactory.h"

#include <boost/python/class.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/list.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/copy_const_reference.hpp>

using Mantid::Kernel::IPropertyManager;
using Mantid::Kernel::Property;
using Mantid::Kernel::PropertyManager;
namespace Registry = Mantid::PythonInterface::Registry;

using namespace boost::python;

namespace
{

}

void export_PropertyManager()
{
  register_ptr_to_python<boost::shared_ptr<PropertyManager>>();
  class_<PropertyManager, bases<IPropertyManager>, boost::noncopyable>("PropertyManager")
   ;
}

#ifdef _MSC_VER
  #pragma warning( default: 4250 )
#endif
