#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidPythonInterface/kernel/Converters/CArrayToNDArray.h"
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"
#include "MantidPythonInterface/kernel/Registry/RegisterSingleValueHandler.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_non_const_reference.hpp>
#include <boost/python/numeric.hpp>

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
   * Determine the sizes of each dimensions
   * @param self :: A reference to the calling object
   * @returns A vector of the dimension sizes
   */
  std::vector<Py_intptr_t> countDimensions(const IMDHistoWorkspace &self)
  {
    const size_t ndims = self.getNumDims();
    std::vector<Py_intptr_t> dims(ndims);
    for(size_t i = 0; i < ndims; ++i)
    {
      dims[i] = static_cast<Py_intptr_t>(self.getDimension(i)->getNBins());
    }
    return dims;
  }

  /**
   * Returns the signal array from the workspace as a numpy array
   * @param self :: A reference to the calling object
   */
  PyObject *getSignalArrayAsNumpyArray(IMDHistoWorkspace &self)
  {
    auto dims = countDimensions(self);
    return WrapReadOnlyNumpy()(self.getSignalArray(), static_cast<int>(dims.size()), &dims[0]);
  }

  /**
   * Returns the error squared array from the workspace as a numpy array
   * @param self :: A reference to the calling object
   */
  PyObject *getErrorSquaredArrayAsNumpyArray(IMDHistoWorkspace &self)
  {
    auto dims = countDimensions(self);
    return WrapReadOnlyNumpy()(self.getErrorSquaredArray(), static_cast<int>(dims.size()), &dims[0]);
  }

  /**
   * Checks the size of the given array against the given MDHistoWorkspace to see if they match. Throws if not
   * @param self :: The calling object
   * @param signal :: The new values
   * @param fnLabel :: A message prefix to pass if the sizes are incorrect
   */
  void throwIfSizeIncorrect(IMDHistoWorkspace & self, const numeric::array & signal,const std::string & fnLabel)
  {
    auto wsShape = countDimensions(self);
    const size_t ndims = wsShape.size();
    auto arrShape = signal.attr("shape");
    if(ndims != static_cast<size_t>(len(arrShape)))
    {
      std::ostringstream os;
      os << fnLabel << ": The number of  dimensions doe not match the current workspace size. Workspace="
         << ndims << " array=" << len(arrShape);
      throw std::invalid_argument(os.str());
    }

    for(size_t i = 0; i < ndims; ++i)
    {
      int arrDim = extract<int>(arrShape[i])();
      if(wsShape[i] != arrDim )
      {
        std::ostringstream os;
        os << fnLabel << ": The dimension size for the " << boost::lexical_cast<std::string>(i) << "th dimension do not match. "
           << "Workspace dimension size=" << wsShape[i] << ", array size=" << arrDim;
        throw std::invalid_argument(os.str());
      }
    }
  }

  /**
   * Set the signal array from a numpy array. This simply loops over the array & sets each value
   * It does not allow the workspace dimensions to be resized, it will throw if the sizes are not
   * correct
   */
  void setSignalArray(IMDHistoWorkspace &self, const numeric::array & signalValues)
  {
    throwIfSizeIncorrect(self, signalValues, "setSignalArray");
    object flattened = signalValues.attr("flat");
    auto length = len(flattened);
    for(auto i = 0; i < length; ++i)
    {
      self.setSignalAt(i, extract<double>(flattened[i])());
    }
  }

  /**
   * Set the square of the errors array from a numpy array. This simply loops over the array & sets each value
   * It does not allow the workspace dimensions to be resized, it will throw if the sizes are not
   * correct
   */
  void setErrorSquaredArray(IMDHistoWorkspace &self, const numeric::array & errorSquared)
  {
    throwIfSizeIncorrect(self, errorSquared, "setErrorSquaredArray");
    object flattened = errorSquared.attr("flat");
    auto length = len(flattened);
    for(auto i = 0; i < length; ++i)
    {
      self.setErrorSquaredAt(i, extract<double>(flattened[i])());
    }
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

    .def("setSignalArray", &setSignalArray,
         "Sets the signal from a numpy array. The sizes must match the current workspace sizes. A ValueError is thrown if not")

    .def("setErrorSquaredArray", &setErrorSquaredArray,
         "Sets the square of the errors from a numpy array. The sizes must match the current workspace sizes. A ValueError is thrown if not")

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

