// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/RegionSelector/RegionSelector.h"
#include "MantidAPI/RegionSelectorObserver.h"
#include "MantidAPI/Workspace.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/Common/Python/Sip.h"

#include <QLayout>
#include <QWidget>
#include <boost/python/extract.hpp>
#include <vector>

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
  if (workspace) {
    options["ws"] = workspace;
  }
  auto constructor = presenterModule().attr("RegionSelector");
  return constructor(*boost::python::tuple(), **options);
}
} // namespace

namespace MantidQt::Widgets {

RegionSelector::RegionSelector(Workspace_sptr const &workspace, QLayout *layout)
    : Python::InstanceHolder(newPresenter(workspace)), m_layout(layout) {
  GlobalInterpreterLock lock;
  auto view = Python::extract<QWidget>(getView());
  constexpr auto MIN_SLICEVIEWER_HEIGHT = 250;
  view->setMinimumHeight(MIN_SLICEVIEWER_HEIGHT);
  m_layout->addWidget(view);
  show();
}

RegionSelector::RegionSelector(RegionSelector &&) = default;

RegionSelector &RegionSelector::operator=(RegionSelector &&) = default;

Common::Python::Object RegionSelector::getView() const {
  GlobalInterpreterLock lock;
  return pyobj().attr("view");
}

void RegionSelector::show() const {
  GlobalInterpreterLock lock;
  getView().attr("show")();
}

void RegionSelector::subscribe(std::shared_ptr<Mantid::API::RegionSelectorObserver> const &notifyee) {
  GlobalInterpreterLock lock;
  boost::python::dict kwargs;
  kwargs["notifyee"] = notifyee;
  pyobj().attr("subscribe")(*boost::python::tuple(), **kwargs);
}

void RegionSelector::updateWorkspace(Workspace_sptr const &workspace) {
  GlobalInterpreterLock lock;
  boost::python::dict kwargs;
  kwargs["workspace"] = workspace;
  pyobj().attr("update_workspace")(*boost::python::tuple(), **kwargs);
}

void RegionSelector::addRectangularRegion() {
  GlobalInterpreterLock lock;
  pyobj().attr("add_rectangular_region")();
}

auto RegionSelector::getRegion() -> Selection {
  GlobalInterpreterLock lock;
  auto pyValues = pyobj().attr("get_region")();
  auto result = Selection();
  for (int i = 0; i < len(pyValues); ++i) {
    result.push_back(boost::python::extract<double>(pyValues[i]));
  }
  return result;
}
} // namespace MantidQt::Widgets
