#include "MantidQtCustomInterfaces/Tomography/ImageStackPreParams.h"
#include "MantidQtCustomInterfaces/Tomography/ImageCoRPresenter.h"
#include "MantidQtCustomInterfaces/Tomography/IImageCoRView.h"

using namespace MantidQt::CustomInterfaces;

namespace MantidQt {
namespace CustomInterfaces {

ImageCoRPresenter::ImageCoRPresenter(IImageCoRView *view)
    : m_view(view), m_model(new ImageStackPreParams()) {
  if (!m_view) {
    throw std::runtime_error("Severe inconsistency found. Presenter created "
                             "with an empty/null view (tomography interface). "
                             "Cannot continue.");
  }
}

ImageCoRPresenter::~ImageCoRPresenter() { cleanup(); }

void ImageCoRPresenter::cleanup() {}

void ImageCoRPresenter::notify(Notification notif) {}

void ImageCoRPresenter::processInit() {}

void ImageCoRPresenter::processBrowseImg() {}

void ImageCoRPresenter::processNewImg() {}

void ImageCoRPresenter::processSelectCoR() {}

void ImageCoRPresenter::processSelectROI() {}

void ImageCoRPresenter::processSelectNormalization() {}

void ImageCoRPresenter::processFinishedCoR() {}

void ImageCoRPresenter::processFinishedROI() {}

void ImageCoRPresenter::processFinishedNormalization() {}

} // namespace CustomInterfaces
} // namespace MantidQt
