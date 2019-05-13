// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/MantidAxes.h"

using Mantid::PythonInterface::GlobalInterpreterLock;
namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * Construct an Axes wrapper around an existing Axes instance
 * @param obj A matplotlib.axes.Axes instance
 */
MantidAxes::MantidAxes(Python::Object pyObj) : Axes{std::move(pyObj)} {}

/**
 * Add a line on the current axes based on the workspace
 * @param workspace
 * @param wkspIndex
 * @param format
 * @param label
 * @return A new Line2D artist object
 */
Line2D MantidAxes::plot(const Mantid::API::MatrixWorkspace_sptr &workspace,
                        const size_t wkspIndex, const QString lineColour,
                        const QString label) {
  using Mantid::API::MatrixWorkspace_sptr;
  using MatrixWorkpaceToPython = Python::ToPythonValue<MatrixWorkspace_sptr>;

  GlobalInterpreterLock lock;
  auto wksp{Python::NewRef(MatrixWorkpaceToPython()(workspace))};
  auto args = Python::NewRef(Py_BuildValue("(O)", wksp.ptr()));
  Python::Dict kwargs;
  kwargs["wkspIndex"] = wkspIndex;
  kwargs["color"] = lineColour.toLatin1().constData();
  kwargs["label"] = label.toLatin1().constData();
  return Line2D{pyobj().attr("plot")(*args, **kwargs)[0]};
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
