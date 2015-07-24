#include "MantidQtCustomInterfaces/Tomography/ImageCoRViewQtGUI.h"
#include "MantidQtCustomInterfaces/Tomography/ImageCoRPresenter.h"

#include "MantidQtAPI/AlgorithmInputHistory.h"

using namespace MantidQt::CustomInterfaces;

#include <QFileDialog>

namespace MantidQt {
namespace CustomInterfaces {

ImageCoRViewQtGUI::ImageCoRViewQtGUI(QWidget *parent)
    : QWidget(parent), IImageCoRView(), m_presenter(NULL) {
  initLayout();
}

void ImageCoRViewQtGUI::initLayout() {
  // setup container ui
  m_ui.setupUi(this);

  setupConnections();

  // presenter that knows how to handle a IImageCoRView should take care
  // of all the logic. Note the view needs to now the concrete presenter here
  m_presenter.reset(new ImageCoRPresenter(this));

  // it will know what compute resources and tools we have available:
  // This view doesn't even know the names of compute resources, etc.
  m_presenter->notify(ImageCoRPresenter::Init);
}

void ImageCoRViewQtGUI::setupConnections() {
  // 'browse' buttons
  connect(m_ui.pushButton_browse_img, SIGNAL(released()), this,
          SLOT(browseImgClicked()));
}

void ImageCoRViewQtGUI::initParams(ImageStackPreParams &params) {
  m_params = params;
}

ImageStackPreParams ImageCoRViewQtGUI::userSelection() const {
  return m_params;
}

void ImageCoRViewQtGUI::showImgOrStack() {}
void ImageCoRViewQtGUI::showImg() {}

void ImageCoRViewQtGUI::browseImgClicked() {
  m_presenter->notify(IImageCoRPresenter::BrowseImgOrStack);
  // get path
  QString fitsStr = QString("Supported formats: FITS, TIFF and PNG "
                            "(*.fits *.fit *.tiff *.tif *.png);;"
                            "FITS, Flexible Image Transport System images "
                            "(*.fits *.fit);;"
                            "TIFF, Tagged Image File Format "
                            "(*.tif *.tiff);;"
                            "PNG, Portable Network Graphics "
                            "(*.png);;"
                            "Other extensions/all files (*.*)");
  QString prevPath =
      MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  QString path(QFileDialog::getOpenFileName(this, tr("Open image file"),
                                            prevPath, fitsStr));
  if (!path.isEmpty()) {
    MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(
        QFileInfo(path).absoluteDir().path());
  } else {
    return;
  }

  m_imgPath = path.toStdString();
  m_presenter->notify(IImageCoRPresenter::NewImgOrStack);
}

} // namespace CustomInterfaces
} // namespace MantidQt
