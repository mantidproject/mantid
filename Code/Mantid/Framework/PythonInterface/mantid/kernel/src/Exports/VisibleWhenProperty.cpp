#include "MantidKernel/VisibleWhenProperty.h"
#include <boost/python/class.hpp>

using namespace Mantid::Kernel;
using namespace boost::python;

void export_VisibleWhenProperty()
{
  class_<VisibleWhenProperty, bases<EnabledWhenProperty>,
         boost::noncopyable>("VisibleWhenProperty", no_init)
    .def(init<std::string, ePropertyCriterion, std::string>(
           (arg("otherPropName"), arg("when"), arg("value")),
            "Enabled otherPropName property when value criterion meets that given by the 'when' argument")
        )

    .def(init<std::string, ePropertyCriterion>(
            (arg("otherPropName"), arg("when")),
            "Enabled otherPropName property when criterion does not require a value, i.e isDefault")
        )
    ;
}

