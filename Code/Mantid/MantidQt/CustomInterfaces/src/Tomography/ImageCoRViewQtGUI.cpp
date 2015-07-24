#include "MantidQtCustomInterfaces/Tomography/ImageCoRViewQtGUI.h"
#include "MantidQtCustomInterfaces/Tomography/ImageCoRPresenter.h"

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

  // presenter that knows how to handle a IImageCoRView should take care
  // of all the logic. Note the view needs to now the concrete presenter here
  m_presenter.reset(new ImageCoRPresenter(this));

  // it will know what compute resources and tools we have available:
  // This view doesn't even know the names of compute resources, etc.
  m_presenter->notify(ImageCoRPresenter::Init);
}

} // namespace CustomInterfaces
} // namespace MantidQt
