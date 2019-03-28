// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/MplCpp/Plot.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidQtWidgets/MplCpp/Python/QHashToDict.h"

using namespace Mantid::API;
using namespace Mantid::PythonInterface;

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

Python::Object plot(std::vector<MatrixWorkspace_sptr> workspaces,
                    boost::optional<std::vector<std::size_t>> spectrum_nums,
                    boost::optional<std::vector<std::size_t>> wksp_indices,
                    boost::optional<Python::Object> fig,
                    boost::optional<QHash<QString, QVariant>> plot_kwargs,
                    boost::optional<QHash<QString, QVariant>> ax_properties,
                    boost::optional<std::string> window_title, bool errors,
                    bool overplot) {
  UNUSED_ARG(workspaces)
  UNUSED_ARG(spectrum_nums)
  UNUSED_ARG(wksp_indices)
  UNUSED_ARG(fig)
  UNUSED_ARG(ax_properties)
  UNUSED_ARG(window_title)
  UNUSED_ARG(errors)
  UNUSED_ARG(overplot)
  auto funcs = PyImport_ImportModule("mantidqt.plotting.functions");
  UNUSED_ARG(funcs)
  GlobalInterpreterLock lock;
  // workspaces, spectrum_nums, wksp_indices, errors, overplot, fig,
  // plot_kwargs, ax_properties, window_title;
  auto dict_plot_kwargs = Python::qHashToDict(plot_kwargs.get());
  //   auto args = Python::NewRef(Py_BuildValue());
  //   auto kwargs = Python::NewRef(Py_BuildValue());
  //   return Python::NewRef(
  //       PyObject_Call(funcs.attr("plot"), args.get(), kwargs.get()));
  return Python::Object();
}
} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
