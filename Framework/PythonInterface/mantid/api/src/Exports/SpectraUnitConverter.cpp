// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/SpectraUnitConverter.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/make_constructor.hpp>

using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::SpectraUnitConverter;
using Mantid::Kernel::DeltaEMode;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(SpectraUnitConverter)

namespace {
/// The workspace is taken as a shared pointer, which the converter keeps, and which from Python holds a reference to
/// the Python workspace object, keeping that alive for as long as the converter too
SpectraUnitConverter *createSpectraUnitConverter(const MatrixWorkspace_sptr &workspace, const std::string &unit,
                                                 const std::string &emode, const object &efixed) {
  const auto fixedEnergy = efixed.is_none() ? std::nullopt : std::optional<double>(extract<double>(efixed)());
  return new SpectraUnitConverter(workspace, unit, DeltaEMode::fromString(emode), fixedEnergy);
}
} // namespace

void export_SpectraUnitConverter() {
  class_<SpectraUnitConverter, boost::noncopyable>(
      "SpectraUnitConverter",
      "Converts X values of a workspace's spectra between its X unit and another unit, as ConvertUnits would, for "
      "the workspace methods that take a unitConverter. The geometry of each spectrum is looked up once, when it is "
      "made, so it is worth keeping while the workspace and unit stay the same. It keeps the workspace alive.",
      no_init)
      .def("__init__",
           make_constructor(&createSpectraUnitConverter, default_call_policies(),
                            (arg("workspace"), arg("unit"), arg("emode") = "Elastic", arg("efixed") = object())))
      .def("unit", &SpectraUnitConverter::unit, arg("self"), return_value_policy<copy_const_reference>(),
           "The unit converted to and from the workspace's X unit.")
      .def("workspaceUnit", &SpectraUnitConverter::workspaceUnit, arg("self"),
           return_value_policy<copy_const_reference>(), "The X unit of the workspace this was made for.");
}
