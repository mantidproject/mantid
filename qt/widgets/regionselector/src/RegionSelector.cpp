// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/RegionSelector/RegionSelector.h"
#include "MantidAPI/RegionSelectorObserver.h"
#include "MantidAPI/Workspace.h"
#include "MantidPythonInterface/core/ErrorHandling.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidQtWidgets/Common/IImageInfoWidget.h"
#include "MantidQtWidgets/Common/ImageInfoWidgetMini.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/Common/Python/Sip.h"

#include <QLayout>
#include <QWidget>
#include <boost/python/extract.hpp>
#include <iostream>
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

Python::Object newPresenter(Workspace_sptr workspace, MantidQt::MantidWidgets::IImageInfoWidget *imageInfoWidget) {
  GlobalInterpreterLock lock;

  boost::python::dict options;
  if (workspace) {
    options["ws"] = workspace;
  }
  if (imageInfoWidget) {
    options["image_info_widget"] = Python::wrap(
        static_cast<MantidQt::MantidWidgets::ImageInfoWidgetMini *>(imageInfoWidget), "ImageInfoWidgetMini");
  }
  auto constructor = presenterModule().attr("RegionSelector");
  return constructor(*boost::python::tuple(), **options);
}
} // namespace

namespace MantidQt::Widgets {

RegionSelector::RegionSelector(Workspace_sptr const &workspace, QLayout *layout,
                               MantidWidgets::IImageInfoWidget *imageInfoWidget)
    : Python::InstanceHolder(newPresenter(workspace, imageInfoWidget)), m_layout(layout) {
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

void RegionSelector::clearWorkspace() {
  GlobalInterpreterLock lock;
  pyobj().attr("clear_workspace")();
}

void RegionSelector::updateWorkspace(Workspace_sptr const &workspace) {
  GlobalInterpreterLock lock;
  boost::python::dict kwargs;
  kwargs["workspace"] = workspace;
  pyobj().attr("update_workspace")(*boost::python::tuple(), **kwargs);
}

void RegionSelector::addRectangularRegion(const std::string &regionType, const std::string &color,
                                          const std::string &hatch) {
  GlobalInterpreterLock lock;
  pyobj().attr("add_rectangular_region")(regionType, color, hatch);
}

void RegionSelector::cancelDrawingRegion() {
  GlobalInterpreterLock lock;
  pyobj().attr("cancel_drawing_region")();
}

void RegionSelector::deselectAllSelectors() {
  GlobalInterpreterLock lock;
  pyobj().attr("deselect_all_selectors")();
}

auto RegionSelector::getRegion(const std::string &regionType) -> Selection {
  GlobalInterpreterLock lock;
  auto pyValues = pyobj().attr("get_region")(regionType);
  auto result = Selection();
  for (int i = 0; i < len(pyValues); ++i) {
    result.push_back(boost::python::extract<double>(pyValues[i]));
  }
  return result;
}

void RegionSelector::displayRectangularRegion(const std::string &regionType, const std::string &color,
                                              const std::string &hatch, const size_t y1, const size_t y2) {
  GlobalInterpreterLock lock;
  pyobj().attr("display_rectangular_region")(regionType, color, hatch, y1, y2);
}
} // namespace MantidQt::Widgets
