// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "PreviewPythonInstrumentView.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/V3D.h"
#include "MantidPythonInterface/core/ErrorHandling.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidPythonInterface/core/WrapPython.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/Common/Python/Sip.h"
#include "ShapeChangedRelay.h"

#include <QWidget>
#include <boost/python/extract.hpp>
#include <boost/python/list.hpp>
#include <boost/python/stl_iterator.hpp>

using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::Workspace_sptr;
using Mantid::PythonInterface::GlobalInterpreterLock;
using Mantid::PythonInterface::PythonException;
using namespace MantidQt::Widgets::Common;

namespace {
Mantid::Kernel::Logger g_log("PreviewPythonInstrumentView");

Python::Object presenterModule() {
  GlobalInterpreterLock lock;
  return Python::Object{
      Python::NewRef(PyImport_ImportModule("instrumentview.isisreflectometry.ReflectometryInstrumentViewPresenter"))};
}

Python::Object newPresenter() {
  GlobalInterpreterLock lock;
  auto constructor = presenterModule().attr("ReflectometryInstrumentViewPresenter");
  return constructor();
}
} // namespace

namespace MantidQt::CustomInterfaces::ISISReflectometry {

PreviewPythonInstrumentView::PreviewPythonInstrumentView(QLayout *layout)
    : Python::InstanceHolder(newPresenter()), m_layout(layout) {
  try {
    GlobalInterpreterLock lock;
    if (m_layout) {
      auto view = Python::extract<QWidget>(getView());
      m_layout->addWidget(view);
      m_relay = std::make_unique<ShapeChangedRelay>(view);
      m_relay->setObjectName("ShapeChangedRelay");
    }
  } catch (boost::python::error_already_set &) {
    g_log.error() << PythonException(true).what() << "\n";
  }
}

PreviewPythonInstrumentView::PreviewPythonInstrumentView(PreviewPythonInstrumentView &&) = default;

PreviewPythonInstrumentView &PreviewPythonInstrumentView::operator=(PreviewPythonInstrumentView &&) = default;

Python::Object PreviewPythonInstrumentView::getView() const {
  GlobalInterpreterLock lock;
  return pyobj().attr("view");
}

void PreviewPythonInstrumentView::setShapeChangedCallback(std::function<void()> callback) {
  if (m_relay)
    m_relay->setCallback(std::move(callback));
}

void PreviewPythonInstrumentView::setLayout(QLayout *layout) {
  if (!layout)
    return;
  m_layout = layout;
  try {
    GlobalInterpreterLock lock;
    auto view = Python::extract<QWidget>(getView());
    m_layout->addWidget(view);
    if (!m_relay) {
      m_relay = std::make_unique<ShapeChangedRelay>(view);
      m_relay->setObjectName("ShapeChangedRelay");
    }
  } catch (boost::python::error_already_set &) {
    g_log.error() << PythonException(true).what() << "\n";
  }
}

void PreviewPythonInstrumentView::updateWorkspace(MatrixWorkspace_sptr &workspace) {
  try {
    GlobalInterpreterLock lock;
    Workspace_sptr workspace2d = std::dynamic_pointer_cast<Mantid::API::Workspace>(workspace);
    if (!workspace2d) {
      g_log.error("Failed to update instrument view because the provided workspace is not a 2D workspace");
      return;
    }
    pyobj().attr("update_workspace")(workspace2d);
  } catch (boost::python::error_already_set &) {
    g_log.error() << PythonException(true).what() << "\n";
  }
}

void PreviewPythonInstrumentView::resetInstView() {
  try {
    GlobalInterpreterLock lock;
    pyobj().attr("reset")();
  } catch (boost::python::error_already_set &) {
    g_log.error() << PythonException(true).what() << "\n";
  }
}

void PreviewPythonInstrumentView::plotInstView() {
  try {
    GlobalInterpreterLock lock;
    pyobj().attr("plot")();
  } catch (boost::python::error_already_set &) {
    g_log.error() << PythonException(true).what() << "\n";
  }
}

void PreviewPythonInstrumentView::setInstViewZoomMode() {
  try {
    GlobalInterpreterLock lock;
    pyobj().attr("set_zoom_mode")();
  } catch (boost::python::error_already_set &) {
    g_log.error() << PythonException(true).what() << "\n";
  }
}

void PreviewPythonInstrumentView::setInstViewSelectRectMode() {
  try {
    GlobalInterpreterLock lock;
    pyobj().attr("set_select_rect_mode")();
  } catch (boost::python::error_already_set &) {
    g_log.error() << PythonException(true).what() << "\n";
  }
}

std::vector<Mantid::detid_t> PreviewPythonInstrumentView::getSelectedDetectorIDs() const {
  try {
    GlobalInterpreterLock lock;
    boost::python::object result = pyobj().attr("selected_detector_ids")();
    boost::python::stl_input_iterator<Mantid::detid_t> begin(result), end;
    return std::vector<Mantid::detid_t>(begin, end);
  } catch (boost::python::error_already_set &) {
    g_log.error() << PythonException(true).what() << "\n";
    return {};
  }
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
