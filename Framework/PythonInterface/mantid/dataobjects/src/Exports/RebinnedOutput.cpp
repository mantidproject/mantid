// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidPythonInterface/api/RegisterWorkspacePtrToPython.h"
#include "MantidPythonInterface/core/Converters/NDArrayToVector.h"
#include "MantidPythonInterface/core/Converters/PySequenceToVector.h"
#include "MantidPythonInterface/core/Converters/WrapWithNDArray.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/Policies/VectorToNumpy.h"

#include <boost/python/class.hpp>

using Mantid::DataObjects::RebinnedOutput;
using Mantid::DataObjects::Workspace2D;
using namespace Mantid::PythonInterface;
using namespace Mantid::PythonInterface::Converters;
using namespace Mantid::PythonInterface::Policies;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(RebinnedOutput)

namespace {
/// Typedef for data access, i.e. dataX,Y,E members
using data_modifier = Mantid::MantidVec &(RebinnedOutput::*)(const std::size_t);

/// return_value_policy for read-only numpy array
using return_readonly_numpy = return_value_policy<VectorRefToNumpy<WrapReadOnly>>;
/// return_value_policy for read-write numpy array
using return_readwrite_numpy = return_value_policy<VectorRefToNumpy<WrapReadWrite>>;

/**
 * Set the F values from an python array-style object
 * @param self :: A reference to the calling object
 * @param wsIndex :: The workspace index for the spectrum to set
 * @param values :: A numpy array, length must match F array length
 */
void setFFromPyObject(RebinnedOutput &self, const size_t wsIndex, const boost::python::object &values) {

  if (NDArray::check(values)) {
    NDArrayToVector<double> converter(values);
    converter.copyTo(self.dataF(wsIndex));
  } else {
    PySequenceToVector<double> converter(values);
    converter.copyTo(self.dataF(wsIndex));
  }
}
} // namespace

void export_RebinnedOutput() {
  class_<RebinnedOutput, bases<Workspace2D>, boost::noncopyable>("RebinnedOutput", no_init)
      .def("readF", &RebinnedOutput::readF, (arg("self"), arg("workspaceIndex")), return_readonly_numpy(),
           "Creates a read-only numpy wrapper "
           "around the original F data at the "
           "given index")
      .def("dataF", (data_modifier)&RebinnedOutput::dataF, return_readwrite_numpy(), args("self", "workspaceIndex"),
           "Creates a writable numpy wrapper around the original F data at the "
           "given index")
      .def("setF", &setFFromPyObject, args("self", "workspaceIndex", "x"),
           "Set F values from a python list or numpy array. It performs a "
           "simple copy into the array")
      .def("scaleF", &RebinnedOutput::scaleF, (arg("self"), arg("scale")), "Scales the fractional area arrays")
      .def("nonZeroF", &RebinnedOutput::nonZeroF, (arg("self")), "Returns if the fractional area is non zero")
      .def("finalize", &RebinnedOutput::finalize, (arg("self"), arg("hasSqrdErrs")),
           "Divides the data/error arrays by the corresponding fractional area "
           "array")
      .def("unfinalize", &RebinnedOutput::unfinalize, (arg("self")),
           "Multiplies the data/error arrays by the corresponding fractional "
           "area array")
      .def("isFinalized", &RebinnedOutput::isFinalized, (arg("self")),
           "Returns if values are normalized to the fractional area array")
      .def("hasSqrdErrors", &RebinnedOutput::hasSqrdErrors, (arg("self")),
           "Returns if squared errors are used with fractional area "
           "normalization")
      .def("setFinalized", &RebinnedOutput::setFinalized, (arg("self"), arg("value")),
           "Sets the value of the is finalized flag")
      .def("setSqrdErrors", &RebinnedOutput::setSqrdErrors, (arg("self"), arg("value")),
           "Sets the value of the squared errors flag");
  // register pointers
  // cppcheck-suppress unusedScopedObject
  RegisterWorkspacePtrToPython<RebinnedOutput>();
}
