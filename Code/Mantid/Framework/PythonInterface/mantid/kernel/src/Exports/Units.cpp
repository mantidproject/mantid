#include "MantidKernel/Unit.h"

#include <boost/python/class.hpp>

using Mantid::Kernel::Unit;
using namespace Mantid::Kernel::Units;
using namespace boost::python;

// We only export the concrete unit classes that
// have additional functionality over the base class

void export_Label()
{
  class_<Label, bases<Unit>, boost::noncopyable>("Label", no_init)
    .def("setLabel", (void (Label::*)(const std::string &,const std::string &))&Label::setLabel, 
         (arg("caption"),arg("label")), "Set the caption (e.g.Temperature) & label (K) on the unit")
    ;

}

