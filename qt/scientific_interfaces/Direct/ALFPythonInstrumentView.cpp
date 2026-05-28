#include "ALFPythonInstrumentView.h"
#include "ALFInstrumentPresenter.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/Common/Python/Sip.h"

#include <QWidget>
#include <boost/python/extract.hpp>
#include <boost/python/list.hpp>
#include <boost/python/stl_iterator.hpp>
#include <stdexcept>

using Mantid::PythonInterface::GlobalInterpreterLock;
using Mantid::PythonInterface::PythonException;
using namespace MantidQt::Widgets::Common;

namespace {
Mantid::Kernel::Logger g_log("ALFPythonInstrumentView");

Python::Object presenterModule() {
  GlobalInterpreterLock lock;
  return Python::Object{Python::NewRef(PyImport_ImportModule("instrumentview.alfview.ALFInstrumentViewPresenter"))};
}

Python::Object newPresenter() {
  GlobalInterpreterLock lock;
  auto constructor = presenterModule().attr("ALFInstrumentViewPresenter");
  return constructor();
}

Python::Object newPresenterWithLogging() {
  try {
    return newPresenter();
  } catch (boost::python::error_already_set &) {
    g_log.error() << "Failed to create ALF Python presenter:\n" << PythonException(true).what() << "\n";
    throw;
  } catch (std::exception const &ex) {
    g_log.error() << "Failed to create ALF Python presenter: " << ex.what() << "\n";
    throw;
  }
}
} // namespace

namespace MantidQt::CustomInterfaces {

ALFPythonInstrumentView::ALFPythonInstrumentView(QWidget *parent)
    : ALFInstrumentViewBase(parent), Python::InstanceHolder(newPresenterWithLogging()) {
  try {
    GlobalInterpreterLock lock;
    pyobj().attr("initialise")();
  } catch (boost::python::error_already_set &) {
    g_log.error() << PythonException(true).what() << "\n";
  } catch (std::exception const &ex) {
    g_log.error() << ex.what() << "\n";
  } catch (...) {
    g_log.error("Unknown exception while initialising ALF Python instrument view");
  }
}

QWidget *ALFPythonInstrumentView::getInstrumentView() {
  try {
    GlobalInterpreterLock lock;
    return Python::extract<QWidget>(pyobj().attr("view"));
  } catch (boost::python::error_already_set &) {
    g_log.error() << PythonException(true).what() << "\n";
    return nullptr;
  } catch (std::exception const &ex) {
    g_log.error() << ex.what() << "\n";
    return nullptr;
  } catch (...) {
    g_log.error("Unknown exception getting ALF Python instrument view widget");
    return nullptr;
  }
}

MantidWidgets::IInstrumentActor const &ALFPythonInstrumentView::getInstrumentActor() const {
  throw std::runtime_error("ALFPythonInstrumentView::getInstrumentActor is not implemented yet");
}

std::vector<DetectorTube> ALFPythonInstrumentView::getSelectedDetectors() const { return {}; }
} // namespace MantidQt::CustomInterfaces
