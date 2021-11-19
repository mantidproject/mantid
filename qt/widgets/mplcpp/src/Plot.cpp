// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/Plot.h"

#include "MantidPythonInterface/core/CallMethod.h"
#include "MantidPythonInterface/core/Converters/ToPyList.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/Common/Python/QHashToDict.h"
#include "MantidQtWidgets/Common/Python/Sip.h"
#include "MantidQtWidgets/MplCpp/Plot.h"
#include <utility>

using namespace Mantid::PythonInterface;
using namespace MantidQt::Widgets::Common;

namespace MantidQt::Widgets::MplCpp {

namespace {

/**
 * @returns The mantidqt.plotting.functions module
 */
Python::Object functionsModule() { return Python::NewRef(PyImport_ImportModule("mantidqt.plotting.functions")); }

/**
 * Construct a Python list from a vector of strings
 * @param workspaces A strings
 * @return A new Python list object
 */
Python::Object constructArgs(const std::vector<std::string> &workspaces) {
  return Python::NewRef(Py_BuildValue("(O)", Converters::ToPyList<std::string>()(workspaces).ptr()));
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
    kwargs["spectrum_nums"] = Converters::ToPyList<int>()(spectrumNums.value());
  } else if (wkspIndices && !spectrumNums) {
    kwargs["wksp_indices"] = Converters::ToPyList<int>()(wkspIndices.value());
  } else {
    throw std::invalid_argument("Passed spectrum numbers and workspace indices, please only pass one, "
                                "with the other being std::nullopt.");
  }

  if (errors)
    kwargs["errors"] = errors.value();
  if (overplot)
    kwargs["overplot"] = overplot.value();
  if (tiled)
    kwargs["tiled"] = tiled.value();
  if (fig)
    kwargs["fig"] = fig.value();
  if (plotKwargs)
    kwargs["plot_kwargs"] = Python::qHashToDict(plotKwargs.value());
  if (axProperties)
    kwargs["ax_properties"] = Python::qHashToDict(axProperties.value());
  if (windowTitle)
    kwargs["window_title"] = windowTitle.value();

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

Python::Object pcolormesh(const QStringList &workspaces, std::optional<Python::Object> fig) {
  GlobalInterpreterLock lock;
  try {
    const auto args = constructArgs(workspaces);
    Python::Dict kwargs;
    if (fig)
      kwargs["fig"] = fig.value();
    return functionsModule().attr("pcolormesh")(*args, **kwargs);
  } catch (Python::ErrorAlreadySet &) {
    throw PythonException();
  }
}

} // namespace MantidQt::Widgets::MplCpp
