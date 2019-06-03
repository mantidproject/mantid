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
/**
 * Construct a Python list from a vector of strings
 * @param workspaces A strings
 * @return A new Python list object
 */
Python::Object constructArgs(const std::vector<std::string> &workspaces) {
  return Python::NewRef(Py_BuildValue(
      "(O)", Converters::ToPyList<std::string>()(workspaces).ptr()));
}

/**
 * Construct a Python list from a QStringList
 * @param workspaces A list of strings
 * @return A new Python list object
 */
Python::Object constructArgs(const QStringList &workspaces) {
  auto sipAPI = Python::Detail::sipAPI();
  auto copy = new QStringList(workspaces);
  auto *sobj = sipAPI->api_convert_from_new_type(
      copy, sipAPI->api_find_type("QStringList"), Py_None);
  return Python::NewRef(Py_BuildValue("(O)", sobj));
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
  Python::Dict kwargs;

  if (spectrumNums && !wkspIndices) {
    kwargs["spectrum_nums"] = Converters::ToPyList<int>()(spectrumNums.get());
  } else if (wkspIndices && !spectrumNums) {
    kwargs["wksp_indices"] = Converters::ToPyList<int>()(wkspIndices.get());
  } else {
    throw std::invalid_argument(
        "Passed spectrum numbers and workspace indices, please only pass one, "
        "with the other being boost::none.");
  }

  kwargs["errors"] = errors;
  kwargs["overplot"] = overplot;
  if (fig)
    kwargs["fig"] = fig.get();
  if (plotKwargs)
    kwargs["plot_kwargs"] = Python::qHashToDict(plotKwargs.get());
  if (axProperties)
    kwargs["ax_properties"] = Python::qHashToDict(axProperties.get());
  if (windowTitle)
    kwargs["window_title"] = windowTitle.get();

  return kwargs;
}

Python::Object plot(const Python::Object &args,
                    boost::optional<std::vector<int>> spectrumNums,
                    boost::optional<std::vector<int>> wkspIndices,
                    boost::optional<Python::Object> fig,
                    boost::optional<QHash<QString, QVariant>> plotKwargs,
                    boost::optional<QHash<QString, QVariant>> axProperties,
                    boost::optional<std::string> windowTitle, bool errors,
                    bool overplot) {
  auto funcsModule =
      Python::NewRef(PyImport_ImportModule("mantidqt.plotting.functions"));
  auto kwargs = constructKwargs(spectrumNums, wkspIndices, fig, plotKwargs,
                                axProperties, windowTitle, errors, overplot);
  try {
    return funcsModule.attr("plot")(*args, **kwargs);
  } catch (Python::ErrorAlreadySet &) {
    throw PythonException();
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
  return plot(constructArgs(workspaces), std::move(spectrumNums),
              std::move(wkspIndices), std::move(fig), std::move(plotKwargs),
              std::move(axProperties), std::move(windowTitle), errors,
              overplot);
}

Python::Object plot(const QStringList &workspaces,
                    boost::optional<std::vector<int>> spectrumNums,
                    boost::optional<std::vector<int>> wkspIndices,
                    boost::optional<Python::Object> fig,
                    boost::optional<QHash<QString, QVariant>> plotKwargs,
                    boost::optional<QHash<QString, QVariant>> axProperties,
                    boost::optional<std::string> windowTitle, bool errors,
                    bool overplot) {
  GlobalInterpreterLock lock;
  return plot(constructArgs(workspaces), std::move(spectrumNums),
              std::move(wkspIndices), std::move(fig), std::move(plotKwargs),
              std::move(axProperties), std::move(windowTitle), errors,
              overplot);
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
