// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/MantidAxes.h"

#include "MantidQtWidgets/Common/Python/QHashToDict.h"

using Mantid::API::MatrixWorkspace_sptr;
using Mantid::PythonInterface::GlobalInterpreterLock;

namespace MantidQt {
namespace Widgets {
namespace Python = Common::Python;
namespace MplCpp {
using MatrixWorkpaceToPython = Python::ToPythonValue<MatrixWorkspace_sptr>;

/**
 * Construct an Axes wrapper around an existing Axes instance
 * @param obj A matplotlib.axes.Axes instance
 */
MantidAxes::MantidAxes(Python::Object pyObj) : Axes{std::move(pyObj)} {}

/**
 * Add a line on the current axes based on the workspace
 * @param workspace A pointer to a workspace containing the data
 * @param wkspIndex The workspace index to plot
 * @param lineColour Set the line colour to this string name
 * @param label A label for the curve
 * @param otherKwargs Other kwargs to use for the line
 * @return A new Line2D artist object
 */
Line2D
MantidAxes::plot(const Mantid::API::MatrixWorkspace_sptr &workspace,
                 const size_t wkspIndex, const QString lineColour,
                 const QString label,
                 const boost::optional<QHash<QString, QVariant>> &otherKwargs) {
  GlobalInterpreterLock lock;
  const auto wksp = Python::NewRef(MatrixWorkpaceToPython()(workspace));
  const auto args = Python::NewRef(Py_BuildValue("(O)", wksp.ptr()));

  Python::Dict kwargs;
  if (otherKwargs)
    kwargs = Python::qHashToDict(otherKwargs.get());
  kwargs["wkspIndex"] = wkspIndex;
  kwargs["color"] = lineColour.toLatin1().constData();
  kwargs["label"] = label.toLatin1().constData();

  return Line2D{pyobj().attr("plot")(*args, **kwargs)[0]};
}

/**
 * Add an errorbar plot on the current axes based on the workspace
 * @param workspace A pointer to a workspace containing the data
 * @param wkspIndex The workspace index to plot
 * @param lineColour Set the line colour to this string name
 * @param label A label for the curve
 * @return A new ErrorbarContainer object
 */
ErrorbarContainer MantidAxes::errorbar(
    const Mantid::API::MatrixWorkspace_sptr &workspace, const size_t wkspIndex,
    const QString lineColour, const QString label,
    const boost::optional<QHash<QString, QVariant>> &otherKwargs) {
  GlobalInterpreterLock lock;
  const auto wksp = Python::NewRef(MatrixWorkpaceToPython()(workspace));
  const auto args = Python::NewRef(Py_BuildValue("(O)", wksp.ptr()));

  Python::Dict kwargs;
  if (otherKwargs)
    kwargs = Python::qHashToDict(otherKwargs.get());
  kwargs["wkspIndex"] = wkspIndex;
  kwargs["color"] = lineColour.toLatin1().constData();
  kwargs["label"] = label.toLatin1().constData();

  return ErrorbarContainer{pyobj().attr("errorbar")(*args, **kwargs)};
}

void MantidAxes::pcolormesh(
    const Mantid::API::MatrixWorkspace_sptr &workspace,
    const boost::optional<QHash<QString, QVariant>> &otherKwargs) {
  GlobalInterpreterLock lock;
  const auto wksp = Python::NewRef(MatrixWorkpaceToPython()(workspace));
  const auto args = Python::NewRef(Py_BuildValue("(O)", wksp.ptr()));

  Python::Dict kwargs;
  if (otherKwargs)
    kwargs = Python::qHashToDict(otherKwargs.get());

  pyobj().attr("pcolormesh")(*args, **kwargs);
}

/**
 * Remove any artists plotted from the given workspace.
 * @param ws A reference to a workspace whose name is used to
 * lookup any artists for removal
 */
void MantidAxes::removeWorkspaceArtists(
    const Mantid::API::MatrixWorkspace_sptr &ws) {
  GlobalInterpreterLock lock;
  try {
    pyobj().attr("remove_workspace_artists")(
        Python::NewRef(MatrixWorkpaceToPython()(ws)));
  } catch (Python::ErrorAlreadySet &) {
    throw Mantid::PythonInterface::PythonException();
  }
}

/**
 * Replace the artists on this axes instance that are based off this workspace
 * @param newWS A reference to the new workspace containing the data
 */
void MantidAxes::replaceWorkspaceArtists(
    const Mantid::API::MatrixWorkspace_sptr &newWS) {
  GlobalInterpreterLock lock;
  pyobj().attr("replace_workspace_artists")(
      Python::NewRef(MatrixWorkpaceToPython()(newWS)));
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
