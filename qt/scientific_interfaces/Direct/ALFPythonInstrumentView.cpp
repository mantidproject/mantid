#include "ALFPythonInstrumentView.h"
#include "ALFInstrumentPresenter.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/Common/Python/Sip.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"

#include <QWidget>
#include <boost/python/extract.hpp>
#include <boost/python/list.hpp>
#include <boost/python/stl_iterator.hpp>
#include <stdexcept>
#include <vector>

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
    : ALFInstrumentViewBase(parent), Python::InstanceHolder(newPresenterWithLogging()) {}

void ALFPythonInstrumentView::setUpInstrument(std::string const &workspaceName) {
  GlobalInterpreterLock lock;
  boost::python::object result = pyobj().attr("update_view")(workspaceName);
  m_workspaceName = workspaceName;
  resetActor();
  observeReplace(true);
}

void ALFPythonInstrumentView::replaceHandle(const std::string &wsName, const Workspace_sptr &) {
  // Only update if replaced workspace is workspace that is open
  if (m_workspaceName != wsName) {
    return;
  }
  resetActor();
}

void ALFPythonInstrumentView::resetActor() {
  // Need to update the actor to refresh with latest data in the workspace
  m_actor = std::make_unique<MantidWidgets::InstrumentActor>(m_workspaceName, *messageHandler);
  m_actor->initialize(false, false);
}

QWidget *ALFPythonInstrumentView::getInstrumentView() {
  try {
    GlobalInterpreterLock lock;
    auto *instrumentView = Python::extract<QWidget>(pyobj().attr("_view"));
    ensureCallbackRelay(instrumentView);
    return instrumentView;
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

void ALFPythonInstrumentView::ensureCallbackRelay(QWidget *instrumentView) {
  if (!instrumentView)
    return;

  if (m_callbackRelay == nullptr || m_callbackRelay->parent() != instrumentView) {
    auto *existingRelay = instrumentView->findChild<ALFPythonCallbackRelay *>(QStringLiteral("ALFPythonCallbackRelay"));
    m_callbackRelay = existingRelay != nullptr ? existingRelay : new ALFPythonCallbackRelay(instrumentView);
  }
  m_callbackRelay->setCallback("notify_whole_tube_selected", [this]() { notifyWholeTubeSelected(); });
}

MantidWidgets::IInstrumentActor const &ALFPythonInstrumentView::getInstrumentActor() const {
  try {
    return *m_actor;
  } catch (boost::python::error_already_set &) {
    g_log.error() << PythonException(true).what() << "\n";
    throw std::runtime_error("ALFPythonInstrumentView::getInstrumentActor not correctly implemented");
  } catch (std::exception const &ex) {
    g_log.error() << ex.what() << "\n";
    throw std::runtime_error("ALFPythonInstrumentView::getInstrumentActor not correctly implemented");
  } catch (...) {
    g_log.error("Unknown exception getting ALF Python instrument view widget");
    throw std::runtime_error("ALFPythonInstrumentView::getInstrumentActor not correctly implemented");
  }
}

void ALFPythonInstrumentView::notifyWholeTubeSelected() {
  m_presenter->notifyTubesSelected(getSelectedDetectors());
  m_presenter->notifyShapeChanged();
  return;
}

std::vector<DetectorTube> ALFPythonInstrumentView::getSelectedDetectors() const {
  try {
    GlobalInterpreterLock lock;
    boost::python::object result = pyobj().attr("selected_detector_indices_by_tube")();
    boost::python::stl_input_iterator<boost::python::object> begin(result), end;
    std::vector<DetectorTube> tubes;
    for (auto it = begin; it != end; ++it) {
      // Convert numpy array to Python list if needed
      boost::python::object listObj = it->attr("tolist")();
      boost::python::stl_input_iterator<std::size_t> innerBegin(listObj), innerEnd;
      tubes.emplace_back(innerBegin, innerEnd);
    }
    return tubes;
  } catch (boost::python::error_already_set &) {
    g_log.error() << PythonException(true).what() << "\n";
    return {};
  } catch (std::exception const &ex) {
    g_log.error() << ex.what() << "\n";
    return {};
  } catch (...) {
    g_log.error("Unknown exception getting ALF Python instrument view widget");
    return {};
  }
}
} // namespace MantidQt::CustomInterfaces
