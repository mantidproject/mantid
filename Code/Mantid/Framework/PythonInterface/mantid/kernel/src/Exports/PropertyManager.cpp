#ifdef _MSC_VER
#pragma warning(disable : 4250) // Disable warning regarding inheritance via
                                // dominance, we have no way around it with the
                                // design
#endif

#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/PropertyManager.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Kernel::IPropertyManager;
using Mantid::Kernel::PropertyManager;

using namespace boost::python;

void export_PropertyManager() {
  typedef boost::shared_ptr<PropertyManager> PropertyManager_sptr;

  // The second argument defines the actual type held within the Python object.
  // This means that when a PropertyManager is constructed in Python it actually used
  // a shared_ptr to the object rather than a raw pointer. This knowledge is used by
  // DataServiceExporter::extractCppValue to assume that it can always extract a shared_ptr
  // type
  class_<PropertyManager, PropertyManager_sptr, bases<IPropertyManager>,
         boost::noncopyable>("PropertyManager");
}

#ifdef _MSC_VER
#pragma warning(default : 4250)
#endif
