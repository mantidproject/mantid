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

void ImageCoRPresenter::notify(Notification notif) {

  switch (notif) {

  case IImageCoRPresenter::Init:
    processInit();
    break;

  case IImageCoRPresenter::BrowseImgOrStack:
    processBrowseImg();
    break;

  case IImageCoRPresenter::NewImgOrStack:
    processNewImg();
    break;

  case IImageCoRPresenter::SelectCoR:
    processSelectCoR();
    break;

  case IImageCoRPresenter::SelectROI:
    processSelectROI();
    break;

  case IImageCoRPresenter::SelectNormalization:
    processSelectNormalization();
    break;

  case IImageCoRPresenter::FinishedCoR:
    processFinishedCoR();
    break;

  case IImageCoRPresenter::FinishedROI:
    processFinishedROI();
    break;

  case IImageCoRPresenter::FinishedNormalization:
    processFinishedNormalization();
    break;
  }
}

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
