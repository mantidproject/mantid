// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/MplCpp/RegionSelector.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/Common/Python/Sip.h"

#include <QWidget>

using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::Workspace_sptr;
using Mantid::PythonInterface::GlobalInterpreterLock;
using namespace MantidQt::Widgets::Common;

namespace {
Python::Object presenterModule() {
  GlobalInterpreterLock lock;
  Python::Object module{Python::NewRef(PyImport_ImportModule("mantidqt.widgets.regionselector.presenter"))};
  return module;
}

Python::Object newPresenter(Workspace_sptr workspace) {
  GlobalInterpreterLock lock;

  boost::python::dict options;
  options["ws"] = workspace;
  options["is_window"] = false;
  auto constructor = presenterModule().attr("RegionSelector");
  return constructor(*boost::python::tuple(), **options);
}
} // namespace

namespace MantidQt::Widgets::MplCpp {

RegionSelector::RegionSelector(MatrixWorkspace_sptr const &workspace, QLayout *layout)
    : Python::InstanceHolder(newPresenter(workspace)), m_layout(layout) {
  GlobalInterpreterLock lock;
  auto view = Python::extract<QWidget>(getView());
  m_layout->addWidget(view);
  show();
}

Common::Python::Object RegionSelector::getView() const {
  GlobalInterpreterLock lock;
  return pyobj().attr("view");
}

void RegionSelector::show() const {
  GlobalInterpreterLock lock;
  getView().attr("show")();
}
} // namespace MantidQt::Widgets::MplCpp