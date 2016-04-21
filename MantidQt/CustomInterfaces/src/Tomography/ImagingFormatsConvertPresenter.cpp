#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtCustomInterfaces/Tomography/IImagingFormatsConvertView.h"
#include "MantidQtCustomInterfaces/Tomography/ImggFormats.h"
#include "MantidQtCustomInterfaces/Tomography/ImagingFormatsConvertPresenter.h"

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
  const std::vector<std::string> formats = {
      shortName(ImggFormats::FITS), shortName(ImggFormats::TIFF),
      shortName(ImggFormats::PNG), shortName(ImggFormats::JPG),
      shortName(ImggFormats::NXTomo)};

  m_view->setFormats(formats);
}

void ImagingFormatsConvertPresenter::processConvert() {
  const std::string inPS = m_view->inputPath();
  const std::string outPS = m_view->outputPath();

}

void ImagingFormatsConvertPresenter::processShutDown() {
  m_view->saveSettings();
}

} // namespace CustomInterfaces
} // namespace MantidQt
