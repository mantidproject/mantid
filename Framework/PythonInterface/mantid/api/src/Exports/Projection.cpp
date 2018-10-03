// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/Projection.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/kernel/Converters/PyObjectToV3D.h"
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/python/class.hpp>
#include <boost/python/copy_non_const_reference.hpp>
#include <boost/python/exec.hpp>
#include <boost/python/import.hpp>
#include <boost/python/make_constructor.hpp>

using namespace Mantid::API;
using namespace Mantid::PythonInterface;
using namespace boost::python;

GNU_DIAG_OFF("strict-aliasing")

namespace {
std::string getUnit(Projection &p, size_t nd) {
  return (p.getUnit(nd) == RLU ? "r" : "a");
}

void setUnit(Projection &p, size_t nd, const std::string &unit) {
  if (unit == "r")
    p.setUnit(nd, RLU);
  else if (unit == "a")
    p.setUnit(nd, INV_ANG);
  else
    throw std::runtime_error("Invalid unit");
}

// This is a bit strange, but it works. The users want x =
// proj.createWorkspace()
// to behave like the simpleapi, i.e. put a workspace named 'x' into the ADS for
// them. To do that kind of black magic we have to introspect from the Python
// side, so that's what we do.
object createWorkspace() {
  // Create a namespace for us to add a function to
  object main = import("__main__");
  object global(main.attr("__dict__"));
  // Add a function to the namespace
  object result = exec(
      "def createWorkspace(proj, OutputWorkspace=None):\n"
      "  '''Create a TableWorkspace using this projection'''\n"
      "  import inspect\n"
      "  from mantid import api, kernel, AnalysisDataService\n"
      "  ws = api.WorkspaceFactory.createTable('TableWorkspace')\n"
      "  ws.addColumn('str', 'name')\n"
      "  ws.addColumn('V3D', 'value')\n"
      "  ws.addColumn('str', 'type')\n"
      "  ws.addColumn('double', 'offset')\n"
      "  for (name, i) in zip('uvw', range(3)):\n"
      "    ws.addRow({\n"
      "              'name': name,\n"
      "              'value': proj.getAxis(i),\n"
      "              'type': proj.getType(i),\n"
      "              'offset': proj.getOffset(i)\n"
      "              })\n"

      "  if OutputWorkspace is None:\n"
      "    lhs = "
      "kernel.funcinspect.process_frame(inspect.currentframe().f_back)\n"
      "    if lhs[0] > 0:\n"
      "      OutputWorkspace = lhs[1][0]\n"
      "    else:\n"
      "      raise RuntimeError('createWorkspace failed to infer a name for its"
      " output projection workspace. Please pass an"
      " OutputWorkspace parameter to it.')\n"
      "  if OutputWorkspace:\n"
      "    AnalysisDataService[OutputWorkspace] = ws\n"

      "  return ws\n"
      "\n",
      global, global);
  // extract the function object from the namespace and return it so it can be
  // bound by Boost::Python.
  return global["createWorkspace"];
}

void projSetAxis(Projection &self, size_t nd, const object &data) {
  self.setAxis(nd, Converters::PyObjectToV3D(data)());
}

Projection_sptr projCtor2(const object &d1, const object &d2) {
  return Projection_sptr(new Projection(Converters::PyObjectToV3D(d1)(),
                                        Converters::PyObjectToV3D(d2)()));
}

Projection_sptr projCtor3(const object &d1, const object &d2,
                          const object &d3) {
  return Projection_sptr(new Projection(Converters::PyObjectToV3D(d1)(),
                                        Converters::PyObjectToV3D(d2)(),
                                        Converters::PyObjectToV3D(d3)()));
}

} // anonymous namespace

void export_Projection() {
  class_<Projection>(
      "Projection",
      init<>("Default constructor creates a two dimensional projection"))
      .def(init<const Mantid::Kernel::V3D &, const Mantid::Kernel::V3D &>(
          "Constructs a 3 dimensional projection, with w as the cross product "
          "of u and v.",
          args("u", "v")))
      .def(init<const Mantid::Kernel::V3D &, const Mantid::Kernel::V3D &,
                const Mantid::Kernel::V3D &>(
          "Constructs a 3 dimensional projection", args("u", "v", "w")))
      .def("__init__",
           make_constructor(&projCtor2, default_call_policies(),
                            (arg("u"), arg("v"))),
           "Constructs a 3 dimensional projection, with w as the cross product "
           "of u and v.")
      .def("__init__",
           make_constructor(&projCtor3, default_call_policies(),
                            (arg("u"), arg("v"), arg("w"))),
           "Constructs a 3 dimensional projection")
      .def("getOffset", &Projection::getOffset, (arg("self"), arg("nd")),
           "Returns the offset for the given dimension", args("dimension"))
      .def("getAxis", &Projection::getAxis, (arg("self"), arg("nd")),
           "Returns the axis for the given dimension", args("dimension"))
      .def("getType", &getUnit, (arg("self"), arg("dimension")),
           "Returns the unit for the given dimension")
      .def("setOffset", &Projection::setOffset,
           (arg("self"), arg("nd"), arg("offset")),
           "Sets the offset for the given dimension",
           args("dimension", "offset"))
      .def("setAxis", &Projection::setAxis,
           (arg("self"), arg("dimension"), arg("axis")),
           "Sets the axis for the given dimension")
      .def("setAxis", &projSetAxis, (arg("self"), arg("nd"), arg("data")),
           "Sets the axis for the given dimension")
      .def("setType", &setUnit, (arg("self"), arg("dimension"), arg("unit")),
           "Sets the unit for the given dimension")
      .add_property(
          "u",
          make_function(
              &Projection::U, return_internal_reference<>(),
              boost::mpl::vector2<Mantid::Kernel::V3D &, Projection &>()),
          make_function(
              boost::bind(&Projection::setAxis, _1, 0, _2),
              default_call_policies(),
              boost::mpl::vector3<void, Projection &, Mantid::Kernel::V3D>()))
      .add_property(
          "v",
          make_function(
              &Projection::V, return_internal_reference<>(),
              boost::mpl::vector2<Mantid::Kernel::V3D &, Projection &>()),
          make_function(
              boost::bind(&Projection::setAxis, _1, 1, _2),
              default_call_policies(),
              boost::mpl::vector3<void, Projection &, Mantid::Kernel::V3D>()))
      .add_property(
          "w",
          make_function(
              &Projection::W, return_internal_reference<>(),
              boost::mpl::vector2<Mantid::Kernel::V3D &, Projection &>()),
          make_function(
              boost::bind(&Projection::setAxis, _1, 2, _2),
              default_call_policies(),
              boost::mpl::vector3<void, Projection &, Mantid::Kernel::V3D>()))
      .add_property(
          "u",
          make_function(
              &Projection::U, return_internal_reference<>(),
              boost::mpl::vector2<Mantid::Kernel::V3D &, Projection &>()),
          make_function(
              boost::bind(&projSetAxis, _1, 0, _2), default_call_policies(),
              boost::mpl::vector3<void, Projection &, const object &>()))
      .add_property(
          "v",
          make_function(
              &Projection::V, return_internal_reference<>(),
              boost::mpl::vector2<Mantid::Kernel::V3D &, Projection &>()),
          make_function(
              boost::bind(&projSetAxis, _1, 1, _2), default_call_policies(),
              boost::mpl::vector3<void, Projection &, const object &>()))
      .add_property(
          "w",
          make_function(
              &Projection::W, return_internal_reference<>(),
              boost::mpl::vector2<Mantid::Kernel::V3D &, Projection &>()),
          make_function(
              boost::bind(&projSetAxis, _1, 2, _2), default_call_policies(),
              boost::mpl::vector3<void, Projection &, const object &>()))
      .def("createWorkspace", createWorkspace(),
           "Create a TableWorkspace representing the projection");
}
