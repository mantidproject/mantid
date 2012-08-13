#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidPythonInterface/kernel/Converters/CArrayToNDArray.h"
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"
#include "MantidPythonInterface/kernel/Registry/RegisterSingleValueHandler.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_non_const_reference.hpp>

using Mantid::API::IMDHistoWorkspace;
using Mantid::API::IMDHistoWorkspace_sptr;
using Mantid::API::IMDWorkspace;
using Mantid::API::MultipleExperimentInfos;
using Mantid::Kernel::DataItem_sptr;
namespace Converters = Mantid::PythonInterface::Converters;

using namespace boost::python;

namespace
{
  /// Convenience typedef
  typedef Converters::CArrayToNDArray<Mantid::signal_t, Converters::WrapReadOnly> WrapReadOnlyNumpy;

  /**
   * Returns the signal array from the workspace as a numpy array
   * @param self :: A reference to the calling object
   */
  PyObject *getSignalArrayAsNumpyArray(IMDHistoWorkspace &self)
  {
    const size_t ndims = self.getNumDims();
    std::vector<Py_intptr_t> dims(ndims);
    for(size_t i = 0; i < ndims; ++i)
    {
      dims[i] = static_cast<Py_intptr_t>(self.getDimension(i)->getNBins());
    }
    return WrapReadOnlyNumpy()(self.getSignalArray(), static_cast<int>(ndims), &dims[0]);
  }

  /**
   * Returns the error squared array from the workspace as a numpy array
   * @param self :: A reference to the calling object
   */
  PyObject *getErrorSquaredArrayAsNumpyArray(IMDHistoWorkspace &self)
  {
    const size_t ndims = self.getNumDims();
    std::vector<Py_intptr_t> dims(ndims);
    for(size_t i = 0; i < ndims; ++i)
    {
      dims[i] = static_cast<Py_intptr_t>(self.getDimension(i)->getNBins());
    }
    return WrapReadOnlyNumpy()(self.getErrorSquaredArray(), static_cast<int>(ndims), &dims[0]);
  }

}

void export_IMDHistoWorkspace()
{
  REGISTER_SHARED_PTR_TO_PYTHON(IMDHistoWorkspace);

  // EventWorkspace class
  class_< IMDHistoWorkspace, bases<IMDWorkspace,MultipleExperimentInfos>, boost::noncopyable >("IMDHistoWorkspace", no_init)
    .def("getSignalArray", &getSignalArrayAsNumpyArray,
         "Returns a read-only numpy array containing the signal values")

    .def("getErrorSquaredArray", &getErrorSquaredArrayAsNumpyArray,
         "Returns a read-only numpy array containing the square of the error values")

    .def("signalAt", &IMDHistoWorkspace::signalAt, return_value_policy<copy_non_const_reference>(),
         "Return a reference to the signal at the linear index")

    .def("errorSquaredAt", &IMDHistoWorkspace::errorSquaredAt, return_value_policy<copy_non_const_reference>(),
        "Return the squared-errors at the linear index")

    .def("setSignalAt", &IMDHistoWorkspace::setSignalAt, "Sets the signal at the specified index.")

    .def("setErrorSquaredAt", &IMDHistoWorkspace::setErrorSquaredAt, "Sets the squared-error at the specified index.")

    .def("setTo", &IMDHistoWorkspace::setTo, "Sets all signals/errors in the workspace to the given values")

    .def("getInverseVolume", &IMDHistoWorkspace::getInverseVolume, return_value_policy< return_by_value >(),
         "Return the inverse of volume of EACH cell in the workspace.")

    .def("getLinearIndex", (size_t(IMDHistoWorkspace::*)(size_t,size_t) const)&IMDHistoWorkspace::getLinearIndex,
         return_value_policy< return_by_value >(), "Get the 1D linear index from the 2D array")

    .def("getLinearIndex", (size_t(IMDHistoWorkspace::*)(size_t,size_t,size_t) const)&IMDHistoWorkspace::getLinearIndex,
        return_value_policy< return_by_value >(), "Get the 1D linear index from the 2D array")

    .def("getLinearIndex", (size_t(IMDHistoWorkspace::*)(size_t,size_t,size_t,size_t) const)&IMDHistoWorkspace::getLinearIndex,
        return_value_policy< return_by_value >(), "Get the 1D linear index from the 2D array")

    .def("getCenter", &IMDHistoWorkspace::getCenter, return_value_policy< return_by_value >(),
         "Return the position of the center of a bin at a given position")
  ;

  REGISTER_SINGLEVALUE_HANDLER(IMDHistoWorkspace_sptr);

}

