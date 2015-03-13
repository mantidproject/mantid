#include "MantidAPI/Projection.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/kernel/Converters/PyObjectToV3D.h"
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/python/class.hpp>
#include <boost/python/copy_non_const_reference.hpp>
#include <boost/python/make_constructor.hpp>

using namespace Mantid::API;
using namespace Mantid::PythonInterface;
using namespace boost::python;

GCC_DIAG_OFF(strict-aliasing)

namespace {
std::string indexToName(size_t i) {
  switch (i) {
  case 0:
    return "u";
  case 1:
    return "v";
  case 2:
    return "w";
  default:
    return "d" + boost::lexical_cast<std::string>(i);
  }
}

std::string getUnit(Projection &p, size_t nd) {
  return (p.getUnit(nd) == RLU ? "r" : "a");
}

void setUnit(Projection &p, size_t nd, std::string unit) {
  if (unit == "r")
    p.setUnit(nd, RLU);
  else if (unit == "a")
    p.setUnit(nd, INV_ANG);
  else
    throw std::runtime_error("Invalid unit");
}

ITableWorkspace_sptr toWorkspace(Projection &p) {
  ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
  auto colName = ws->addColumn("str", "name");
  auto colValue = ws->addColumn("str", "value");
  auto colType = ws->addColumn("str", "type");
  auto colOffset = ws->addColumn("double", "offset");

  for (size_t i = 0; i < 3; ++i) {
    TableRow row = ws->appendRow();
    row << indexToName(i) << p.getAxis(i).toString() << getUnit(p, i)
        << p.getOffset(i);
  }

  return ws;
}

void projSetAxis(Projection &self, size_t nd, const object& data) {
  self.setAxis(nd, Converters::PyObjectToV3D(data)());
}

Projection_sptr projCtor2(const object &d1, const object &d2) {
  return Projection_sptr(new Projection(Converters::PyObjectToV3D(d1)(),
                                        Converters::PyObjectToV3D(d2)()));
}

Projection_sptr projCtor3(
  const object& d1,
  const object& d2,
  const object& d3) {
  return Projection_sptr(new Projection(
    Converters::PyObjectToV3D(d1)(),
    Converters::PyObjectToV3D(d2)(),
    Converters::PyObjectToV3D(d3)())
  );
}

} //anonymous namespace

void export_Projection()
{
  class_<Projection>(
    "Projection",
    init<>("Default constructor creates a two dimensional projection")
  )
  .def(
    init<const V3D&,const V3D&>
    ("Constructs a 3 dimensional projection, with w as the cross product "
     "of u and v.", args("u","v")))
  .def(
    init<const V3D&,const V3D&,const V3D&>
    ("Constructs a 3 dimensional projection", args("u","v","w")))
  .def(
    "__init__",
    make_constructor(&projCtor2),
    "Constructs a 3 dimensional projection, with w as the cross product "
    "of u and v.")
  .def(
    "__init__",
    make_constructor(&projCtor3),
    "Constructs a 3 dimensional projection")
  .def(
    "getOffset",
    &Projection::getOffset,
    "Returns the offset for the given dimension",
    args("dimension"))
  .def(
    "getAxis",
    &Projection::getAxis,
    "Returns the axis for the given dimension", args("dimension"))
  .def(
    "getType",
    &getUnit,
    "Returns the unit for the given dimension", args("dimension"))
  .def(
    "setOffset",
    &Projection::setOffset,
    "Sets the offset for the given dimension",
    args("dimension", "offset"))
  .def(
    "setAxis",
    &Projection::setAxis,
    "Sets the axis for the given dimension",
    args("dimension", "axis"))
  .def(
    "setAxis",
    &projSetAxis,
    "Sets the axis for the given dimension",
    args("dimension", "axis")
  )
  .def(
    "setType",
    &setUnit,
    "Sets the unit for the given dimension",
    args("dimension", "unit")
  )
  .add_property("u",
    make_function(
      &Projection::U,
      return_internal_reference<>(),
      boost::mpl::vector2<V3D&, Projection&>()
    ),
    make_function(
      boost::bind(&Projection::setAxis, _1, 0, _2),
      default_call_policies(),
      boost::mpl::vector3<void, Projection&, V3D>()
    )
  )
  .add_property("v",
    make_function(
      &Projection::V,
      return_internal_reference<>(),
      boost::mpl::vector2<V3D&, Projection&>()
    ),
    make_function(
      boost::bind(&Projection::setAxis, _1, 1, _2),
      default_call_policies(),
      boost::mpl::vector3<void, Projection&, V3D>()
    )
  )
  .add_property("w",
    make_function(
      &Projection::W,
      return_internal_reference<>(),
      boost::mpl::vector2<V3D&, Projection&>()
    ),
    make_function(
      boost::bind(&Projection::setAxis, _1, 2, _2),
      default_call_policies(),
      boost::mpl::vector3<void, Projection&, V3D>()
    )
  )
  .add_property("u",
    make_function(
      &Projection::U,
      return_internal_reference<>(),
      boost::mpl::vector2<V3D&, Projection&>()
    ),
    make_function(
      boost::bind(&projSetAxis, _1, 0, _2),
      default_call_policies(),
      boost::mpl::vector3<void, Projection&, const object&>()
    )
  )
  .add_property("v",
    make_function(
      &Projection::V,
      return_internal_reference<>(),
      boost::mpl::vector2<V3D&, Projection&>()
    ),
    make_function(
      boost::bind(&projSetAxis, _1, 1, _2),
      default_call_policies(),
      boost::mpl::vector3<void, Projection&, const object&>()
    )
  )
  .add_property("w",
    make_function(
      &Projection::W,
      return_internal_reference<>(),
      boost::mpl::vector2<V3D&, Projection&>()
    ),
    make_function(
      boost::bind(&projSetAxis, _1, 2, _2),
      default_call_policies(),
      boost::mpl::vector3<void, Projection&, const object&>()
    )
  )
  .def(
    "toWorkspace",
    toWorkspace,
    "Create a TableWorkspace representing the projection"
  )
  ;
}
