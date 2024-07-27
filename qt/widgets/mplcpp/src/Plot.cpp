// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/Plot.h"

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidPythonInterface/core/CallMethod.h"
#include "MantidPythonInterface/core/Converters/ToPyList.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/Common/Python/QHashToDict.h"
#include "MantidQtWidgets/Common/Python/Sip.h"
#include "MantidQtWidgets/MplCpp/Plot.h"

#include <utility>

using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::Workspace_sptr;
using namespace Mantid::PythonInterface;
using namespace MantidQt::Widgets::Common;

namespace MantidQt::Widgets::MplCpp {

namespace {

/**
 * @returns The mantidqt.plotting.functions module
 */
Python::Object functionsModule() { return Python::NewRef(PyImport_ImportModule("mantidqt.plotting.functions")); }

Python::Object sviewerModule() {
  return Python::NewRef(PyImport_ImportModule("mantidqt.widgets.sliceviewer.presenters.presenter"));
}

Python::Object userConfModule() { return Python::NewRef(PyImport_ImportModule("workbench.config")); }

/**
 * Construct a Python list from a vector of strings
 * @param workspaces A strings
 * @return A new Python list object
 */
Python::Object constructArgs(const std::vector<std::string> &workspaces) {
  return Python::NewRef(Py_BuildValue("(O)", Converters::ToPyList<std::string>()(workspaces).ptr()));
}

/**
 * Construct a Python list from a vector of workspace pointers
 * @param workspaces A list of MatrixWorkspace_sptr
 * @return A new Python list object
 */
Python::Object constructArgs(const std::vector<MatrixWorkspace_sptr> &workspaces) {
  return Python::NewRef(Py_BuildValue("(O)", Converters::ToPyList<MatrixWorkspace_sptr>()(workspaces).ptr()));
}

/**
 * Construct a Python list from a QStringList
 * @param workspaces A list of strings
 * @return A new Python list object
 */
Python::Object constructArgs(const QStringList &workspaces) {
  const auto sipAPI = Python::Detail::sipAPI();
  const auto copy = new QStringList(workspaces);
  const auto *sobj = sipAPI->api_convert_from_new_type(copy, sipAPI->api_find_type("QStringList"), Py_None);
  return Python::NewRef(Py_BuildValue("(O)", sobj));
}

/**
 * Construct kwargs list for the plot function
 */
Python::Object constructKwargs(std::optional<std::vector<int>> spectrumNums,
                               std::optional<std::vector<int>> wkspIndices, std::optional<Python::Object> fig,
                               std::optional<QHash<QString, QVariant>> plotKwargs,
                               std::optional<QHash<QString, QVariant>> axProperties,
                               std::optional<std::string> windowTitle, std::optional<bool> errors,
                               std::optional<bool> overplot, std::optional<bool> tiled) {
  // Make sure to decide whether spectrum numbers or workspace indices
  Python::Dict kwargs;

  if (spectrumNums && !wkspIndices) {
    kwargs["spectrum_nums"] = Converters::ToPyList<int>()(*spectrumNums);
  } else if (wkspIndices && !spectrumNums) {
    kwargs["wksp_indices"] = Converters::ToPyList<int>()(*wkspIndices);
  } else {
    throw std::invalid_argument("Passed spectrum numbers and workspace indices, please only pass one, "
                                "with the other being std::nullopt.");
  }

  if (errors)
    kwargs["errors"] = *errors;
  if (overplot)
    kwargs["overplot"] = *overplot;
  if (tiled)
    kwargs["tiled"] = *tiled;
  if (fig)
    kwargs["fig"] = *fig;
  if (plotKwargs)
    kwargs["plot_kwargs"] = Python::qHashToDict(*plotKwargs);
  if (axProperties)
    kwargs["ax_properties"] = Python::qHashToDict(*axProperties);
  if (windowTitle)
    kwargs["window_title"] = *windowTitle;

  return std::move(kwargs);
}

Python::Object plot(const Python::Object &args, std::optional<std::vector<int>> spectrumNums,
                    std::optional<std::vector<int>> wkspIndices, std::optional<Python::Object> fig,
                    std::optional<QHash<QString, QVariant>> plotKwargs,
                    std::optional<QHash<QString, QVariant>> axProperties, std::optional<std::string> windowTitle,
                    bool errors, bool overplot, bool tiled) {
  const auto kwargs =
      constructKwargs(std::move(spectrumNums), std::move(wkspIndices), std::move(fig), std::move(plotKwargs),
                      std::move(axProperties), std::move(windowTitle), errors, overplot, tiled);
  try {
    return functionsModule().attr("plot")(*args, **kwargs);
  } catch (Python::ErrorAlreadySet &) {
    throw PythonException();
  }
}

} // namespace

Python::Object plot(const std::vector<std::string> &workspaces, std::optional<std::vector<int>> spectrumNums,
                    std::optional<std::vector<int>> wkspIndices, std::optional<Python::Object> fig,
                    std::optional<QHash<QString, QVariant>> plotKwargs,
                    std::optional<QHash<QString, QVariant>> axProperties, std::optional<std::string> windowTitle,
                    bool errors, bool overplot, bool tiled) {
  GlobalInterpreterLock lock;
  return plot(constructArgs(workspaces), std::move(spectrumNums), std::move(wkspIndices), std::move(fig),
              std::move(plotKwargs), std::move(axProperties), std::move(windowTitle), errors, overplot, tiled);
}

Python::Object plot(const QStringList &workspaces, std::optional<std::vector<int>> spectrumNums,
                    std::optional<std::vector<int>> wkspIndices, std::optional<Python::Object> fig,
                    std::optional<QHash<QString, QVariant>> plotKwargs,
                    std::optional<QHash<QString, QVariant>> axProperties, std::optional<std::string> windowTitle,
                    bool errors, bool overplot, bool tiled) {
  GlobalInterpreterLock lock;
  return plot(constructArgs(workspaces), std::move(spectrumNums), std::move(wkspIndices), std::move(fig),
              std::move(plotKwargs), std::move(axProperties), std::move(windowTitle), errors, overplot, tiled);
}

Python::Object plot(const std::vector<MatrixWorkspace_sptr> &workspaces, std::optional<std::vector<int>> spectrumNums,
                    std::optional<std::vector<int>> wkspIndices, std::optional<Python::Object> fig,
                    std::optional<QHash<QString, QVariant>> plotKwargs,
                    std::optional<QHash<QString, QVariant>> axProperties, std::optional<std::string> windowTitle,
                    bool errors, bool overplot, bool tiled) {
  GlobalInterpreterLock lock;
  return plot(constructArgs(workspaces), std::move(spectrumNums), std::move(wkspIndices), std::move(fig),
              std::move(plotKwargs), std::move(axProperties), std::move(windowTitle), errors, overplot, tiled);
}

Python::Object pcolormesh(const QStringList &workspaces, std::optional<Python::Object> fig) {
  GlobalInterpreterLock lock;
  try {
    const auto args = constructArgs(workspaces);
    Python::Dict kwargs;
    if (fig)
      kwargs["fig"] = *fig;
    return functionsModule().attr("pcolormesh")(*args, **kwargs);
  } catch (Python::ErrorAlreadySet &) {
    throw PythonException();
  }
}

Python::Object surface(const QStringList &workspaces, std::optional<Python::Object> fig) {
  GlobalInterpreterLock lock;
  try {
    const auto args = constructArgs(workspaces);
    Python::Dict kwargs;
    if (fig)
      kwargs["fig"] = *fig;
    return functionsModule().attr("plot_surface")(*args, **kwargs);
  } catch (Python::ErrorAlreadySet &) {
    throw PythonException();
  }
}

Python::Object sliceviewer(const Workspace_sptr &workspace) {
  GlobalInterpreterLock lock;
  try {
    const Python::Object py_workspace = Python::NewRef(Python::ToPythonValue<Workspace_sptr>()(workspace));
    const Python::Object args = Python::NewRef(Py_BuildValue("(O)", py_workspace.ptr()));
    // Get a ref to the CONF singleton
    const auto conf = userConfModule().attr("CONF");
    Python::Dict kwargs;
    kwargs["conf"] = conf;

    auto sw = sviewerModule().attr("SliceViewer")(*args, **kwargs);
    return sw.attr("show_view")();
  } catch (Python::ErrorAlreadySet &) {
    throw PythonException();
  }
}

} // namespace MantidQt::Widgets::MplCpp
