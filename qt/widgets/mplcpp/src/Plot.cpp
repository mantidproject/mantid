// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/MplCpp/Plot.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"

using namespace Mantid::API;
using namespace Mantid::PythonInterface;

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

Python::Object plot(std::vector<MatrixWorkspace_sptr> workspaces,
                    boost::optional<std::vector<std::size_t>> spectrum_nums,
                    boost::optional<std::vector<std::size_t>> wksp_indices,
                    boost::optional<Python::Object> fig,
                    boost::optional<QMap<QString, QVariant>> plot_kwargs,
                    boost::optional<QMap<QString, QVariant>> ax_properties,
                    boost::optional<std::string> window_title, bool errors,
                    bool overplot) {
  auto funcs = PyImport_ImportModule("mantidqt.plotting.functions");
  GlobalInterpreterLock lock;
  // workspaces, spectrum_nums, wksp_indices, errors, overplot, fig,
  // plot_kwargs, ax_properties, window_title;
  auto args = Python::NewRef(Py_BuildValue());
  auto kwargs = Python::NewRef(Py_BuildValue());
  return Python::NewRef(
      PyObject_Call(funcs.attr("plot"), args.get(), kwargs.get()));
}
} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
