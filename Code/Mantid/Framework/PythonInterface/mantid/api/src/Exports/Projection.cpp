#include "MantidAPI/Projection.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/WarningSuppressions.h"
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/python/class.hpp>
#include <boost/python/copy_non_const_reference.hpp>

using namespace Mantid::API;
using namespace Mantid::API;
using namespace boost::python;

namespace
{
  std::string indexToName(size_t i)
  {
    switch(i)
    {
      case 0: return "u";
      case 1: return "v";
      case 2: return "w";
      default: return "d" + boost::lexical_cast<std::string>(i);
    }
  }

  std::string getUnit(Projection& p, size_t nd)
  {
    return (p.getUnit(nd) == RLU ? "r" : "a");
  }

  void setUnit(Projection& p, size_t nd, std::string unit)
  {
    if(unit == "r")
      p.setUnit(nd, RLU);
    else if(unit == "a")
      p.setUnit(nd, INV_ANG);
    else
      throw std::runtime_error("Invalid unit");
  }

  ITableWorkspace_sptr toWorkspace(Projection& p)
  {
    if(p.getNumDims() > 3)
      throw std::runtime_error("Only 2 or 3 dimensional projections can be converted to a workspace.");

    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    auto colName = ws->addColumn("str", "name");
    auto colValue = ws->addColumn("str", "value");
    auto colType = ws->addColumn("str", "type");
    auto colOffset = ws->addColumn("double", "offset");

    for(size_t i = 0; i < p.getNumDims(); ++i)
    {
      TableRow row = ws->appendRow();
      row << indexToName(i) << p.getAxis(i).toString(",") << getUnit(p, i) << p.getOffset(i);
    }

    return ws;
  }
}

GCC_DIAG_OFF(strict-aliasing)
void export_Projection()
{
  class_<Projection>("Projection", init<>("Default constructor creates a two dimensional projection"))
    .def(init<size_t>("Constructs an n-dimensional projection", args("num_dimensions")))
    .def(init<const VMD&,const VMD&>("Constructs a 2 dimensional projection", args("u","v")))
    .def(init<const VMD&,const VMD&,const VMD&>("Constructs a 3 dimensional projection", args("u","v","w")))
    .def(init<const VMD&,const VMD&,const VMD&,const VMD&>("Constructs a 4 dimensional projection", args("u","v","w","x")))
    .def(init<const VMD&,const VMD&,const VMD&,const VMD&,const VMD&>("Constructs a 5 dimensional projection", args("u","v","w","x","y")))
    .def(init<const VMD&,const VMD&,const VMD&,const VMD&,const VMD&,const VMD&>("Constructs a 6 dimensional projection", args("u","v","w","x","y","z")))
    .def("getNumDims", &Projection::getNumDims, "Returns the number of dimensions in the projection")
    .def("getOffset", &Projection::getOffset, "Returns the offset for the given dimension", args("dimension"))
    .def("getAxis", &Projection::getAxis, "Returns the axis for the given dimension", args("dimension"))
    .def("getType", &getUnit, "Returns the unit for the given dimension", args("dimension"))
    .def("setOffset", &Projection::setOffset, "Sets the offset for the given dimension", args("dimension", "offset"))
    .def("setAxis", &Projection::setAxis, "Sets the axis for the given dimension", args("dimension", "axis"))
    .def("setType", &setUnit, "Sets the unit for the given dimension", args("dimension", "unit"))
    .add_property("u",
        make_function(&Projection::U, return_internal_reference<>(), boost::mpl::vector2<VMD&, Projection&>()),
        make_function(boost::bind(&Projection::setAxis, _1, 0, _2), default_call_policies(), boost::mpl::vector3<void, Projection&, VMD>())
    )
    .add_property("v",
        make_function(&Projection::V, return_internal_reference<>(), boost::mpl::vector2<VMD&, Projection&>()),
        make_function(boost::bind(&Projection::setAxis, _1, 1, _2), default_call_policies(), boost::mpl::vector3<void, Projection&, VMD>())
    )
    .add_property("w",
        make_function(&Projection::W, return_internal_reference<>(), boost::mpl::vector2<VMD&, Projection&>()),
        make_function(boost::bind(&Projection::setAxis, _1, 2, _2), default_call_policies(), boost::mpl::vector3<void, Projection&, VMD>())
    )
    .def("toWorkspace", toWorkspace, "Create a TableWorkspace representing the projection")
    ;
}
GCC_DIAG_ON(strict-aliasing)
