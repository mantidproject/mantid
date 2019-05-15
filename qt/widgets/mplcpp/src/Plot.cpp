// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/MplCpp/Plot.h"
#include "MantidPythonInterface/core/CallMethod.h"
#include "MantidPythonInterface/core/Converters/ToPyList.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/Common/Python/QHashToDict.h"

using namespace Mantid::PythonInterface;
using namespace MantidQt::Widgets::Common;

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

namespace {
Python::Object constructArgs(const std::vector<std::string> &workspaces) {
  Python::Object workspaceList =
      Converters::ToPyList<std::string>()(workspaces);
  return Python::NewRef(Py_BuildValue("(O)", workspaceList.ptr()));
}

Python::Object
constructKwargs(boost::optional<std::vector<int>> spectrumNums,
                boost::optional<std::vector<int>> wkspIndices,
                boost::optional<Python::Object> fig,
                boost::optional<QHash<QString, QVariant>> plotKwargs,
                boost::optional<QHash<QString, QVariant>> axProperties,
                boost::optional<std::string> windowTitle, bool errors,
                bool overplot) {
  // Make sure to decide whether spectrum numbers or workspace indices
  bool useSpecNums;
  if (spectrumNums && !wkspIndices) {
    useSpecNums = true;
  } else if (wkspIndices && !spectrumNums) {
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
  if (plotKwargs) {
    pythonPlotKwargs = Python::qHashToDict(plotKwargs.get());
  }
  if (axProperties) {
    pythonAxProperties = Python::qHashToDict(axProperties.get());
  }
  if (windowTitle) {
    pythonWindowTitle =
        Python::NewRef(Py_BuildValue("s", windowTitle.get().c_str()));
  }
  if (fig) {
    pythonFig = fig.get();
  }

  if (useSpecNums) {
    // Using specNums
    pythonSpectrumNums = Converters::ToPyList<int>()(spectrumNums.get());

    return Python::NewRef(Py_BuildValue(
        "{sOsOsOsOsOsOsO}", "spectrum_nums", pythonSpectrumNums.ptr(), "errors",
        pythonErrors.ptr(), "overplot", pythonOverplot.ptr(), "fig",
        pythonFig.ptr(), "plot_kwargs", pythonPlotKwargs.ptr(), "ax_properties",
        pythonAxProperties.ptr(), "window_title", pythonWindowTitle.ptr()));
  } else {
    // Using wkspIndices
    pythonWkspIndices = Converters::ToPyList<int>()(wkspIndices.get());

    return Python::NewRef(Py_BuildValue(
        "{sOsOsOsOsOsOsO}", "wksp_indices", pythonWkspIndices.ptr(), "errors",
        pythonErrors.ptr(), "overplot", pythonOverplot.ptr(), "fig",
        pythonFig.ptr(), "plot_kwargs", pythonPlotKwargs.ptr(), "ax_properties",
        pythonAxProperties.ptr(), "window_title", pythonWindowTitle.ptr()));
  }
}
} // namespace

Python::Object plot(const std::vector<std::string> &workspaces,
                    boost::optional<std::vector<int>> spectrumNums,
                    boost::optional<std::vector<int>> wkspIndices,
                    boost::optional<Python::Object> fig,
                    boost::optional<QHash<QString, QVariant>> plotKwargs,
                    boost::optional<QHash<QString, QVariant>> axProperties,
                    boost::optional<std::string> windowTitle, bool errors,
                    bool overplot) {
  GlobalInterpreterLock lock;
  auto funcsModule =
      Python::NewRef(PyImport_ImportModule("mantidqt.plotting.functions"));
  auto args = constructArgs(workspaces);
  auto kwargs = constructKwargs(spectrumNums, wkspIndices, fig, plotKwargs,
                                axProperties, windowTitle, errors, overplot);
  try {
    return funcsModule.attr("plot")(*args, **kwargs);
  } catch (Python::ErrorAlreadySet &) {
    throw PythonException();
  }
}
} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt