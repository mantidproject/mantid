#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtCustomInterfaces/Tomography/ImagingFormatsConvertPresenter.h"
#include "MantidQtCustomInterfaces/Tomography/IImagingFormatsConvertView.h"

using namespace MantidQt::CustomInterfaces;

namespace MantidQt {
namespace CustomInterfaces {

namespace {
Mantid::Kernel::Logger g_log("ImagingFormatsConvert");
}

ImagingFormatsConvertPresenter::ImagingFormatsConvertPresenter(
    IImagingFormatsConvertView *view)
    : m_view(view) {
  if (!m_view) {
    throw std::runtime_error(
        "Severe inconsistency found. Presenter created "
        "with an empty/null view (formats conversion interface). "
        "Cannot continue.");
  }
}

ImagingFormatsConvertPresenter::~ImagingFormatsConvertPresenter() { cleanup(); }

void ImagingFormatsConvertPresenter::cleanup() {}

void ImagingFormatsConvertPresenter::notify(Notification notif) {

  switch (notif) {

  case IImagingFormatsConvertPresenter::Init:
    processInit();
    break;

  case IImagingFormatsConvertPresenter::Convert:
    processConvert();
    break;

  case IImagingFormatsConvertPresenter::ShutDown:
    processShutDown();
    break;
  }
}

void ImagingFormatsConvertPresenter::processInit() {
  // m_view->setParams(p);
}

void ImagingFormatsConvertPresenter::processConvert() {
  // m_view->setParams(p);
}

void ImagingFormatsConvertPresenter::processShutDown() {
  m_view->saveSettings();
}

} // namespace CustomInterfaces
} // namespace MantidQt
