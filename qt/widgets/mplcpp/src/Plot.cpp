// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/MplCpp/Plot.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/Common/Python/QHashToDict.h"

using namespace Mantid::PythonInterface;

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

namespace {
/**
 * Construct a Python list object from the passed vector
 *
 * @tparam T intended to be a std::vector of a type that is a type convertable
 * to the pythonType passed
 * @param vector
 * @param pythonType Any of the types here:
 * https://docs.python.org/2/c-api/arg.html
 * @return Python::Object Python List
 */
template <class T>
PyObject *constructPythonListFromVectorOfTypeT(T vector,
                                               std::string pythonType) {
  const auto vectorSize = vector.size();
  PyObject *pythonList = PyList_New(vectorSize);
  for (auto i = 0u; i < vectorSize; ++i) {
    PyObject *pythonVectorItem;
    pythonVectorItem = Py_BuildValue(pythonType.c_str(), vector[i]);
    PyList_SetItem(pythonList, i, pythonVectorItem);
  }
  return pythonList;
}
PyObject *
constructPythonListFromVectorOfStrings(std::vector<std::string> vector) {
  const auto vectorSize = vector.size();
  PyObject *pythonList = PyList_New(vectorSize);
  for (auto i = 0u; i < vectorSize; ++i) {
    PyObject *pythonVectorItem;
    pythonVectorItem = Py_BuildValue("s", vector[i].c_str());

    PyList_SetItem(pythonList, i, pythonVectorItem);
  }
  return pythonList;
}
} // namespace

Python::Object constructArgs(std::vector<std::string> workspaces) {
  return Python::NewRef(
      Py_BuildValue("(O)", constructPythonListFromVectorOfStrings(workspaces)));
}

Python::Object
constructKwargs(boost::optional<std::vector<int>> spectrum_nums,
                boost::optional<std::vector<int>> wksp_indices,
                boost::optional<Python::Object> fig,
                boost::optional<QHash<QString, QVariant>> plot_kwargs,
                boost::optional<QHash<QString, QVariant>> ax_properties,
                boost::optional<std::string> window_title, bool errors,
                bool overplot) {
  // Make sure to decide whether spectrum numbers or workspace indices
  bool useSpecNums;
  if (spectrum_nums && !wksp_indices) {
    useSpecNums = true;
  } else if (wksp_indices) {
    useSpecNums = false;
  } else {
    throw std::invalid_argument(
        "Passed spectrum numbers and workspace indices, please only pass one, "
        "with the other being boost::none.");
  }

  // Declare all variables at this scope before potential assignment
  Python::Object pythonSpectrumNums;
  Python::Object pythonWkspIndices;
  Python::Object pythonFig;
  Python::Object pythonPlotKwargs;
  Python::Object pythonAxProperties;
  Python::Object pythonWindowTitle;

  // These values are defaulted so not purely optional
  Python::Object pythonErrors = Python::NewRef(Py_BuildValue("b", errors));
  Python::Object pythonOverplot = Python::NewRef(Py_BuildValue("b", overplot));

  // Check the optional variables that are purely optional
  if (plot_kwargs) {
    pythonPlotKwargs = Python::qHashToDict(plot_kwargs.get());
  }
  if (ax_properties) {
    pythonAxProperties = Python::qHashToDict(ax_properties.get());
  }
  if (window_title) {
    pythonWindowTitle =
        Python::NewRef(Py_BuildValue("s", window_title.get().c_str()));
  }
  if (fig) {
    pythonFig = fig.get();
  }

  if (useSpecNums) {
    // Using specNums
    pythonSpectrumNums =
        Python::NewRef(constructPythonListFromVectorOfTypeT<std::vector<int>>(
            spectrum_nums.get(), "i"));

    return Python::NewRef(Py_BuildValue(
        "{sOsOsOsOsOsOsO}", "spectrum_nums", pythonSpectrumNums.ptr(), "errors",
        pythonErrors.ptr(), "overplot", pythonOverplot.ptr(), "fig",
        pythonFig.ptr(), "plot_kwargs", pythonPlotKwargs.ptr(), "ax_properties",
        pythonAxProperties.ptr(), "window_title", pythonWindowTitle.ptr()));
  } else {
    // Using wkspIndices
    pythonWkspIndices =
        Python::NewRef(constructPythonListFromVectorOfTypeT<std::vector<int>>(
            wksp_indices.get(), "i"));

    return Python::NewRef(Py_BuildValue(
        "{sOsOsOsOsOsOsO}", "wksp_indices", pythonWkspIndices.ptr(), "errors",
        pythonErrors.ptr(), "overplot", pythonOverplot.ptr(), "fig",
        pythonFig.ptr(), "plot_kwargs", pythonPlotKwargs.ptr(), "ax_properties",
        pythonAxProperties.ptr(), "window_title", pythonWindowTitle.ptr()));
  }
}

Python::Object plot(std::vector<std::string> workspaces,
                    boost::optional<std::vector<int>> spectrum_nums,
                    boost::optional<std::vector<int>> wksp_indices,
                    boost::optional<Python::Object> fig,
                    boost::optional<QHash<QString, QVariant>> plot_kwargs,
                    boost::optional<QHash<QString, QVariant>> ax_properties,
                    boost::optional<std::string> window_title, bool errors,
                    bool overplot) {
  GlobalInterpreterLock lock;
  PyObject *functionsString =
      PyString_FromString("mantidqt.plotting.functions");
  PyObject *funcsModule = PyImport_Import(functionsString);
  PyObject *plotFunc = PyObject_GetAttrString(funcsModule, "plot");
  auto args = constructArgs(workspaces);
  auto kwargs = constructKwargs(spectrum_nums, wksp_indices, fig, plot_kwargs,
                                ax_properties, window_title, errors, overplot);
  PyObject *returnedFig = PyObject_Call(plotFunc, args.ptr(), kwargs.ptr());
  return Python::NewRef(returnedFig);
}
} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt