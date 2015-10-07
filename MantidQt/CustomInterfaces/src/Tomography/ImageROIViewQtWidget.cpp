#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/AlgorithmRunner.h"

#include "MantidQtCustomInterfaces/Tomography/ImageROIViewQtWidget.h"
#include "MantidQtCustomInterfaces/Tomography/ImageROIPresenter.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QSettings>

namespace MantidQt {
namespace CustomInterfaces {

// this would be more like a CustomWidget if it's eventually moved there
const std::string ImageROIViewQtWidget::m_settingsGroup =
    "CustomInterfaces/ImageROIView";

ImageROIViewQtWidget::ImageROIViewQtWidget(QWidget *parent)
    : QWidget(parent), IImageROIView(), m_imgWidth(0), m_imgHeight(0),
      m_selectionState(SelectNone), m_presenter(NULL) {
  initLayout();

  // using an event filter. might be worth refactoring into a specific
  // QLabel + selection of ROI+NormArea+CoR class
  // not using Qwt Pickers to avoid Qwt version issues..
  m_ui.label_img->installEventFilter(this);
}

void ImageROIViewQtWidget::setParams(ImageStackPreParams &params) {
  m_params = params;
  setParamWidgets(m_params);
}

ImageStackPreParams ImageROIViewQtWidget::userSelection() const {
  return m_params;
}

void ImageROIViewQtWidget::changeSelectionState(
    const IImageROIView::SelectionState &state) {
  m_selectionState = state;
}

void ImageROIViewQtWidget::showStack(const std::string & /*path*/) {
  // TODO:
  // a) load as proper stack of images workspace - this can only be done when
  //    we have a firt working version of the "lean MD workspace". This method
  //    would then load into one workspace of such type.
  // b) load as workspace group - this is done in the overloaded method below

  // enableParamWidgets(true);
}

void ImageROIViewQtWidget::showStack(Mantid::API::WorkspaceGroup_sptr &wsg) {
  if (0 == wsg->size())
    return;

  m_stack = wsg;

  m_ui.horizontalScrollBar_img_stack->setEnabled(true);
  m_ui.horizontalScrollBar_img_stack->setMinimum(0);
  m_ui.horizontalScrollBar_img_stack->setMaximum(
      static_cast<int>(m_stack->size() - 1));

  size_t width = 0, height = 0;
  try {
    MatrixWorkspace_sptr ws =
        boost::dynamic_pointer_cast<MatrixWorkspace>(wsg->getItem(0));
    if (!ws)
      return;
    width = ws->blocksize();
    height = ws->getNumberHistograms();
  } catch (std::exception &e) {
    QMessageBox::warning(this, "Cannot load image information",
                         "There was a problem while "
                         " trying to find the size of the image: " +
                             QString::fromStdString(e.what()));
  }

  showProjection(m_stack, 0);
  initParamWidgets(width, height);
  refreshROIetAl();
  enableParamWidgets(true);
}

void ImageROIViewQtWidget::showProjection(
    const Mantid::API::WorkspaceGroup_sptr &wsg, size_t idx) {

  showProjectionImage(wsg, idx);
  refreshROIetAl();

  // give name, set up scroll/slider
  std::string name;
  try {
    MatrixWorkspace_sptr ws =
        boost::dynamic_pointer_cast<MatrixWorkspace>(wsg->getItem(idx));
    if (!ws)
      return;
    name = ws->run().getLogData("run_title")->value();
  } catch (std::exception &e) {
    QMessageBox::warning(this, "Cannot load image information",
                         "There was a problem while "
                         " trying to find the name of the image: " +
                             QString::fromStdString(e.what()));
  }
  m_ui.label_img_name->setText(QString::fromStdString(name));

  const size_t numPics = wsg->size();
  m_ui.lineEdit_img_seq->setText(
      QString::fromStdString("1/" + boost::lexical_cast<std::string>(numPics)));
  m_ui.horizontalScrollBar_img_stack->setValue(static_cast<int>(idx));
}

void ImageROIViewQtWidget::userWarning(const std::string &err,
                                       const std::string &description) {
  QMessageBox::warning(this, QString::fromStdString(err),
                       QString::fromStdString(description), QMessageBox::Ok,
                       QMessageBox::Ok);
}

void ImageROIViewQtWidget::userError(const std::string &err,
                                     const std::string &description) {
  QMessageBox::critical(this, QString::fromStdString(err),
                        QString::fromStdString(description), QMessageBox::Ok,
                        QMessageBox::Ok);
}

std::string ImageROIViewQtWidget::askImgOrStackPath() {
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
  QString path(QFileDialog::getExistingDirectory(
      this, tr("Open stack of images"), prevPath, QFileDialog::ShowDirsOnly));
  if (!path.isEmpty()) {
    MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(path);
  }

  return path.toStdString();
}

void ImageROIViewQtWidget::saveSettings() const {
  QSettings qs;
  qs.beginGroup(QString::fromStdString(m_settingsGroup));

  qs.setValue("interface-win-geometry", saveGeometry());
  qs.endGroup();
}

void ImageROIViewQtWidget::resetCoR() {
  int midx =
      (m_ui.spinBox_cor_x->minimum() + m_ui.spinBox_cor_x->maximum()) / 2;
  m_ui.spinBox_cor_x->setValue(midx);
  int midy =
      (m_ui.spinBox_cor_y->minimum() + m_ui.spinBox_cor_y->maximum()) / 2;
  m_ui.spinBox_cor_y->setValue(midy);
}

void ImageROIViewQtWidget::resetROI() {
  m_ui.spinBox_roi_top_x->setValue(0);
  m_ui.spinBox_roi_top_y->setValue(0);
  m_ui.spinBox_roi_bottom_x->setValue(m_ui.spinBox_roi_bottom_x->maximum());
  m_ui.spinBox_roi_bottom_y->setValue(m_ui.spinBox_roi_bottom_y->maximum());
}

void ImageROIViewQtWidget::resetNormArea() {
  m_ui.spinBox_norm_top_x->setValue(0);
  m_ui.spinBox_norm_top_y->setValue(0);
  m_ui.spinBox_norm_bottom_x->setValue(0);
  m_ui.spinBox_norm_bottom_y->setValue(0);
}

void ImageROIViewQtWidget::initLayout() {
  // setup container ui
  m_ui.setupUi(this);

  QList<int> sizes;
  sizes.push_back(1000);
  sizes.push_back(100);
  m_ui.splitter_main_horiz->setSizes(sizes);

  sizes.clear();
  sizes.push_back(10);
  sizes.push_back(1000);
  m_ui.splitter_img_vertical->setSizes(sizes);
  m_ui.horizontalScrollBar_img_stack->setEnabled(false);
  m_ui.lineEdit_img_seq->setText("---");

  enableParamWidgets(false);

  setupConnections();

  initParamWidgets(1, 1);
  grabCoRFromWidgets();
  grabROIFromWidgets();
  grabNormAreaFromWidgets();

  // presenter that knows how to handle a IImageROIView should take care
  // of all the logic. Note the view needs to now the concrete presenter here
  m_presenter.reset(new ImageROIPresenter(this));

  // it will know what compute resources and tools we have available:
  // This view doesn't even know the names of compute resources, etc.
  m_presenter->notify(ImageROIPresenter::Init);
}

void ImageROIViewQtWidget::setupConnections() {

  // 'browse' buttons
  connect(m_ui.pushButton_browse_img, SIGNAL(released()), this,
          SLOT(browseImgClicked()));

  connect(m_ui.pushButton_cor, SIGNAL(released()), this, SLOT(corClicked()));
  connect(m_ui.pushButton_cor_reset, SIGNAL(released()), this,
          SLOT(corResetClicked()));

  connect(m_ui.pushButton_roi, SIGNAL(released()), this, SLOT(roiClicked()));
  connect(m_ui.pushButton_roi_reset, SIGNAL(released()), this,
          SLOT(roiResetClicked()));

  connect(m_ui.pushButton_norm_area, SIGNAL(released()), this,
          SLOT(normAreaClicked()));
  connect(m_ui.pushButton_norm_area_reset, SIGNAL(released()), this,
          SLOT(normAreaResetClicked()));

  // image sequence scroll/slide:
  connect(m_ui.horizontalScrollBar_img_stack, SIGNAL(valueChanged(int)), this,
          SLOT(updateFromImagesSlider(int)));

  // parameter (points) widgets
  connect(m_ui.spinBox_cor_x, SIGNAL(valueChanged(int)), this,
          SLOT(valueUpdatedCoR(int)));
  connect(m_ui.spinBox_cor_y, SIGNAL(valueChanged(int)), this,
          SLOT(valueUpdatedCoR(int)));

  connect(m_ui.spinBox_roi_top_x, SIGNAL(valueChanged(int)), this,
          SLOT(valueUpdatedROI(int)));
  connect(m_ui.spinBox_roi_top_y, SIGNAL(valueChanged(int)), this,
          SLOT(valueUpdatedROI(int)));
  connect(m_ui.spinBox_roi_bottom_x, SIGNAL(valueChanged(int)), this,
          SLOT(valueUpdatedROI(int)));
  connect(m_ui.spinBox_roi_bottom_y, SIGNAL(valueChanged(int)), this,
          SLOT(valueUpdatedROI(int)));

  connect(m_ui.spinBox_norm_top_x, SIGNAL(valueChanged(int)), this,
          SLOT(valueUpdatedNormArea(int)));
  connect(m_ui.spinBox_norm_top_y, SIGNAL(valueChanged(int)), this,
          SLOT(valueUpdatedNormArea(int)));
  connect(m_ui.spinBox_norm_bottom_x, SIGNAL(valueChanged(int)), this,
          SLOT(valueUpdatedNormArea(int)));
  connect(m_ui.spinBox_norm_bottom_y, SIGNAL(valueChanged(int)), this,
          SLOT(valueUpdatedNormArea(int)));
}

void ImageROIViewQtWidget::valueUpdatedCoR(int) {
  grabCoRFromWidgets();
  refreshROIetAl();
}

void ImageROIViewQtWidget::valueUpdatedROI(int) {
  grabROIFromWidgets();
  refreshROIetAl();
}

void ImageROIViewQtWidget::valueUpdatedNormArea(int) {
  grabNormAreaFromWidgets();
  refreshROIetAl();
}

/**
 * Parameter values from spin box widgets => coordinate parameters
 * data member
 */
void ImageROIViewQtWidget::grabCoRFromWidgets() {
  m_params.cor = Mantid::Kernel::V2D(m_ui.spinBox_cor_x->value(),
                                     m_ui.spinBox_cor_y->value());
}

void ImageROIViewQtWidget::grabROIFromWidgets() {
  m_params.roi =
      std::make_pair(Mantid::Kernel::V2D(m_ui.spinBox_roi_top_x->value(),
                                         m_ui.spinBox_roi_top_y->value()),
                     Mantid::Kernel::V2D(m_ui.spinBox_roi_bottom_x->value(),
                                         m_ui.spinBox_roi_bottom_y->value()));
}

void ImageROIViewQtWidget::grabNormAreaFromWidgets() {
  m_params.normalizationRegion =
      std::make_pair(Mantid::Kernel::V2D(m_ui.spinBox_norm_top_x->value(),
                                         m_ui.spinBox_norm_top_y->value()),
                     Mantid::Kernel::V2D(m_ui.spinBox_norm_bottom_x->value(),
                                         m_ui.spinBox_norm_bottom_y->value()));
}

/**
 * Updates the image view with the current image index and selection
 * of parameters (ROI, normalization area, CoR). This needs to be used
 * for every event that modifies the current image and/or selection of
 * parameters.
 */
void ImageROIViewQtWidget::refreshROIetAl() {

  const QPixmap *pp = m_ui.label_img->pixmap();
  if (!pp)
    return;

  QPixmap toDisplay(*m_basePixmap.get());
  QPainter painter(&toDisplay);

  // TODO: display settings / nicer symbol?

  QPen penCoR(Qt::red);
  painter.setPen(penCoR);
  painter.drawLine(static_cast<int>(m_params.cor.X() - 5),
                   static_cast<int>(m_params.cor.Y()),
                   static_cast<int>(m_params.cor.X() + 5),
                   static_cast<int>(m_params.cor.Y()));
  painter.drawLine(static_cast<int>(m_params.cor.X()),
                   static_cast<int>(m_params.cor.Y() - 5),
                   static_cast<int>(m_params.cor.X()),
                   static_cast<int>(m_params.cor.Y() + 5));

  QPen penROI(Qt::green);
  painter.setPen(penROI);
  painter.drawRect(
      static_cast<int>(m_params.roi.first.X()),
      static_cast<int>(m_params.roi.first.Y()),
      static_cast<int>(m_params.roi.second.X() - m_params.roi.first.X()),
      static_cast<int>(m_params.roi.second.Y() - m_params.roi.first.Y()));

  QPen penNA(Qt::yellow);
  painter.setPen(penNA);
  painter.drawRect(static_cast<int>(m_params.normalizationRegion.first.X()),
                   static_cast<int>(m_params.normalizationRegion.first.Y()),
                   static_cast<int>(m_params.normalizationRegion.second.X() -
                                    m_params.normalizationRegion.first.X()),
                   static_cast<int>(m_params.normalizationRegion.second.Y() -
                                    m_params.normalizationRegion.first.Y()));

  m_ui.label_img->setPixmap(toDisplay);
}

void ImageROIViewQtWidget::refreshCoR() {
  const QPixmap *pp = m_ui.label_img->pixmap();
  if (!pp)
    return;

  grabCoRFromWidgets();

  QPixmap toDisplay(*m_basePixmap.get());
  QPainter painter(&toDisplay);
  QPen pen(Qt::red);
  painter.setPen(pen);
  painter.drawLine(static_cast<int>(m_params.cor.X() - 5),
                   static_cast<int>(m_params.cor.Y()),
                   static_cast<int>(m_params.cor.X() + 5),
                   static_cast<int>(m_params.cor.Y()));
  painter.drawLine(static_cast<int>(m_params.cor.X()),
                   static_cast<int>(m_params.cor.Y() - 5),
                   static_cast<int>(m_params.cor.X()),
                   static_cast<int>(m_params.cor.Y() + 5));
  m_ui.label_img->setPixmap(toDisplay);
}

void ImageROIViewQtWidget::refreshROI() {
  const QPixmap *pp = m_ui.label_img->pixmap();
  if (!pp)
    return;

  grabROIFromWidgets();
  // TODO: display proper symbol

  // QPixmap const *pm = m_ui.label_img->pixmap();
  QPixmap toDisplay(*m_basePixmap.get());
  QPainter painter(&toDisplay);
  QPen pen(Qt::green);
  painter.setPen(pen);
  painter.drawRect(
      static_cast<int>(m_params.roi.first.X()),
      static_cast<int>(m_params.roi.first.Y()),
      static_cast<int>(m_params.roi.second.X() - m_params.roi.first.X()),
      static_cast<int>(m_params.roi.second.Y() - m_params.roi.first.Y()));
  m_ui.label_img->setPixmap(toDisplay);
}

void ImageROIViewQtWidget::refreshNormArea() {
  // TODO: display proper symbol
  const QPixmap *pp = m_ui.label_img->pixmap();
  if (!pp)
    return;

  grabNormAreaFromWidgets();

  // QPixmap const *pm = m_ui.label_img->pixmap();
  QPixmap toDisplay(*m_basePixmap.get());
  QPainter painter(&toDisplay);
  QPen pen(Qt::yellow);
  painter.setPen(pen);
  painter.drawRect(static_cast<int>(m_params.normalizationRegion.first.X()),
                   static_cast<int>(m_params.normalizationRegion.first.Y()),
                   static_cast<int>(m_params.normalizationRegion.second.X() -
                                    m_params.normalizationRegion.first.X()),
                   static_cast<int>(m_params.normalizationRegion.second.Y() -
                                    m_params.normalizationRegion.first.Y()));
  m_ui.label_img->setPixmap(toDisplay);
}

void ImageROIViewQtWidget::enableParamWidgets(bool enable) {
  m_ui.groupBox_cor->setEnabled(enable);
  m_ui.groupBox_roi->setEnabled(enable);
  m_ui.groupBox_norm->setEnabled(enable);
}

void ImageROIViewQtWidget::initParamWidgets(size_t maxWidth, size_t maxHeight) {
  m_imgWidth = static_cast<int>(maxWidth);
  m_imgHeight = static_cast<int>(maxHeight);

  m_ui.spinBox_cor_x->setMinimum(0);
  m_ui.spinBox_cor_x->setMaximum(m_imgWidth - 1);
  m_ui.spinBox_cor_y->setMinimum(0);
  m_ui.spinBox_cor_y->setMaximum(m_imgHeight - 1);
  resetCoR();

  m_ui.spinBox_roi_top_x->setMinimum(0);
  m_ui.spinBox_roi_top_x->setMaximum(m_imgWidth - 1);
  m_ui.spinBox_roi_top_y->setMinimum(0);
  m_ui.spinBox_roi_top_y->setMaximum(m_imgHeight - 1);

  m_ui.spinBox_roi_bottom_x->setMinimum(0);
  m_ui.spinBox_roi_bottom_x->setMaximum(m_imgWidth - 1);
  m_ui.spinBox_roi_bottom_y->setMinimum(0);
  m_ui.spinBox_roi_bottom_y->setMaximum(m_imgHeight - 1);

  resetROI();

  m_ui.spinBox_norm_top_x->setMinimum(0);
  m_ui.spinBox_norm_top_x->setMaximum(m_imgWidth - 1);
  m_ui.spinBox_norm_top_y->setMinimum(0);
  m_ui.spinBox_norm_top_y->setMaximum(m_imgHeight - 1);

  m_ui.spinBox_norm_bottom_x->setMinimum(0);
  m_ui.spinBox_norm_bottom_x->setMaximum(m_imgWidth - 1);
  m_ui.spinBox_norm_bottom_y->setMinimum(0);
  m_ui.spinBox_norm_bottom_y->setMaximum(m_imgHeight - 1);

  resetNormArea();
}

void ImageROIViewQtWidget::setParamWidgets(ImageStackPreParams &params) {
  m_ui.spinBox_cor_x->setValue(static_cast<int>(params.cor.X()));
  m_ui.spinBox_cor_y->setValue(static_cast<int>(params.cor.Y()));

  m_ui.spinBox_roi_top_x->setValue(static_cast<int>(params.roi.first.X()));
  m_ui.spinBox_roi_top_y->setValue(static_cast<int>(params.roi.first.Y()));

  m_ui.spinBox_roi_bottom_x->setValue(static_cast<int>(params.roi.second.X()));
  m_ui.spinBox_roi_bottom_y->setValue(static_cast<int>(params.roi.second.Y()));

  m_ui.spinBox_norm_top_x->setValue(
      static_cast<int>(params.normalizationRegion.first.X()));
  m_ui.spinBox_norm_top_y->setValue(
      static_cast<int>(params.normalizationRegion.first.Y()));

  m_ui.spinBox_norm_bottom_x->setValue(
      static_cast<int>(params.normalizationRegion.second.X()));
  m_ui.spinBox_norm_bottom_y->setValue(
      static_cast<int>(params.normalizationRegion.second.Y()));
}

void ImageROIViewQtWidget::corClicked() {
  m_presenter->notify(IImageROIPresenter::SelectCoR);
}

void ImageROIViewQtWidget::corResetClicked() {
  m_presenter->notify(IImageROIPresenter::ResetCoR);
  refreshROIetAl();
}

void ImageROIViewQtWidget::roiClicked() {
  m_presenter->notify(IImageROIPresenter::SelectROI);
}

void ImageROIViewQtWidget::roiResetClicked() {
  m_presenter->notify(IImageROIPresenter::ResetROI);
  refreshROIetAl();
}

void ImageROIViewQtWidget::normAreaClicked() {
  m_presenter->notify(IImageROIPresenter::SelectNormalization);
}

void ImageROIViewQtWidget::normAreaResetClicked() {
  m_presenter->notify(IImageROIPresenter::ResetNormalization);
  refreshROIetAl();
}

void ImageROIViewQtWidget::browseImgClicked() {
  m_presenter->notify(IImageROIPresenter::BrowseImgOrStack);
}

void ImageROIViewQtWidget::updateFromImagesSlider(int /* current */) {
  m_presenter->notify(IImageROIPresenter::UpdateImgIndex);
}

size_t ImageROIViewQtWidget::currentImgIndex() const {
  return m_ui.horizontalScrollBar_img_stack->value();
}

void ImageROIViewQtWidget::updateImgWithIndex(size_t idx) {
  int max = m_ui.horizontalScrollBar_img_stack->maximum();
  int current = m_ui.horizontalScrollBar_img_stack->value();

  showProjection(m_stack, idx);
  m_ui.lineEdit_img_seq->setText(
      QString::fromStdString(boost::lexical_cast<std::string>(current + 1) +
                             "/" + boost::lexical_cast<std::string>(max + 1)));
}

void ImageROIViewQtWidget::showProjectionImage(
    const Mantid::API::WorkspaceGroup_sptr &wsg, size_t idx) {

  MatrixWorkspace_sptr ws;
  try {
    ws = boost::dynamic_pointer_cast<MatrixWorkspace>(wsg->getItem(idx));
    if (!ws)
      return;
  } catch (std::exception &e) {
    QMessageBox::warning(
        this, "Cannot load image",
        "There was a problem while trying to find the image data: " +
            QString::fromStdString(e.what()));
  }

  const size_t MAXDIM = 2048 * 16;
  size_t width;
  try {
    width = boost::lexical_cast<size_t>(ws->run().getLogData("Axis1")->value());
    // TODO: add a settings option for this (like max mem allocation for
    // images)?
    if (width >= MAXDIM)
      width = MAXDIM;
  } catch (std::exception &e) {
    QMessageBox::critical(this, "Cannot load image",
                          "There was a problem while trying to "
                          "find the width of the image: " +
                              QString::fromStdString(e.what()));
    return;
  }

  size_t height;
  try {
    height =
        boost::lexical_cast<size_t>(ws->run().getLogData("Axis2")->value());
    if (height >= MAXDIM)
      height = MAXDIM;
  } catch (std::exception &e) {
    QMessageBox::critical(this, "Cannot load image",
                          "There was a problem while trying to "
                          "find the height of the image: " +
                              QString::fromStdString(e.what()));
    return;
  }

  // images are loaded as 1 histogram == 1 pixel (1 bin per histogram):
  if (height != ws->getNumberHistograms() || width != ws->blocksize()) {
    QMessageBox::critical(
        this, "Image dimensions do not match in the input image workspace",
        "Could not load the expected "
        "number of rows and columns.");
    return;
  }
  // find min and max to scale pixel values
  double min = std::numeric_limits<double>::max(),
         max = std::numeric_limits<double>::min();
  for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
    for (size_t j = 0; j < ws->blocksize(); ++j) {
      const double &v = ws->readY(i)[j];
      if (v < min)
        min = v;
      if (v > max)
        max = v;
    }
  }
  if (min >= max) {
    QMessageBox::warning(
        this, "Empty image!",
        "The image could be loaded but it contains "
        "effectively no information, all pixels have the same value.");
    // black picture
    QPixmap pix(static_cast<int>(width), static_cast<int>(height));
    pix.fill(QColor(0, 0, 0));
    m_ui.label_img->setPixmap(pix);
    m_ui.label_img->show();
    m_basePixmap.reset(new QPixmap(pix));
    return;
  }

  // load / transfer image into a QImage
  QImage rawImg(QSize(static_cast<int>(width), static_cast<int>(height)),
                QImage::Format_RGB32);
  const double max_min = max - min;
  const double scaleFactor = 255.0 / max_min;
  for (size_t yi = 0; yi < width; ++yi) {
    for (size_t xi = 0; xi < width; ++xi) {
      const double &v = ws->readY(yi)[xi];
      // color the range min-max in gray scale. To apply different color
      // maps you'd need to use rawImg.setColorTable() or similar.
      const int scaled = static_cast<int>(scaleFactor * (v - min));
      QRgb vRgb = qRgb(scaled, scaled, scaled);
      rawImg.setPixel(static_cast<int>(xi), static_cast<int>(yi), vRgb);
    }
  }

  // paint and show image
  // direct from image
  QPixmap pix = QPixmap::fromImage(rawImg);
  m_ui.label_img->setPixmap(pix);
  m_ui.label_img->show();
  m_basePixmap.reset(new QPixmap(pix));
  // Alternative, drawing with a painter:
  // QPixmap pix(static_cast<int>(width), static_cast<int>(height));
  // QPainter painter;
  // painter.begin(&pix);
  // painter.drawImage(0, 0, rawImg);
  // painter.end();
  // m_ui.label_img->setPixmap(pix);
  // m_ui.label_img->show();
  // m_basePixmap.reset(new QPixmap(pix));
}

/**
 * Qt events filter for the mouse click and click&drag events that are
 * used to select points and rectangles. Part of the logic of the
 * selection is handled here. The test on the presenter can only test
 * the begin and end of the selection. For full testability (including
 * the mouse interaction), this method should be implemented fully in
 * terms of notifications to the presenter. This would require a bunch
 * of new notifications in IImageROIPresenter, and making at least all
 * the mouseUpdateCoR, mouseUpdateROICorners12, mouseXXX methods
 * public in this view interface. This can be considered at a later
 * stage.
 *
 * @param obj object concerned by the event
 * @param event event received (mouse click, release, move, etc.)
 **/
bool ImageROIViewQtWidget::eventFilter(QObject *obj, QEvent *event) {
  // quick ignore
  if (IImageROIView::SelectNone == m_selectionState)
    return false;

  if (m_ui.label_img == obj) {

    QPoint p = m_ui.label_img->mapFromGlobal(QCursor::pos());
    int x = p.x();
    int y = p.y();
    // ignore potential clicks outside of the image
    if (x >= m_imgWidth || y >= m_imgHeight || x < 0 || y < 0)
      return false;

    auto type = event->type();
    if (type == QEvent::MouseButtonPress) {

      if (IImageROIView::SelectCoR == m_selectionState) {
        mouseUpdateCoR(x, y);
      } else if (IImageROIView::SelectROIFirst == m_selectionState) {
        mouseUpdateROICorners12(x, y);
      } else if (IImageROIView::SelectNormAreaFirst == m_selectionState) {
        mouseUpdateNormAreaCorners12(x, y);
      }
    } else if (type == QEvent::MouseMove) {

      if (IImageROIView::SelectROISecond == m_selectionState) {
        mouseUpdateROICorner2(x, y);
      } else if (IImageROIView::SelectNormAreaSecond == m_selectionState) {
        mouseUpdateNormAreaCorner2(x, y);
      }
    } else if (type == QEvent::MouseButtonRelease) {

      if (IImageROIView::SelectROISecond == m_selectionState) {
        mouseFinishROI(x, y);
      } else if (IImageROIView::SelectNormAreaSecond == m_selectionState) {
        mouseFinishNormArea(x, y);
      }
    }
  }
  // pass on the event up to the parent class
  return false;
}

/**
 * Parameter values from mouse position (at a relevant event like
 * first click, or last release) => spin box widgets, AND coordinate
 * parameters data member. This grabs the Center of Rotation (CoR)
 *
 * @param x position on x axis (local to the image)
 * @param y position on y axis (local to the image)
 */
void ImageROIViewQtWidget::grabCoRFromMousePoint(int x, int y) {
  m_params.cor = Mantid::Kernel::V2D(x, y);
  m_ui.spinBox_cor_x->setValue(x);
  m_ui.spinBox_cor_y->setValue(y);
}

void ImageROIViewQtWidget::grabROICorner1FromMousePoint(int x, int y) {
  m_params.roi.first = Mantid::Kernel::V2D(x, y);
  m_ui.spinBox_roi_top_x->setValue(x);
  m_ui.spinBox_roi_top_y->setValue(y);
}

void ImageROIViewQtWidget::grabROICorner2FromMousePoint(int x, int y) {
  m_params.roi.second = Mantid::Kernel::V2D(x, y);
  m_ui.spinBox_roi_bottom_x->setValue(x);
  m_ui.spinBox_roi_bottom_y->setValue(y);
}

void ImageROIViewQtWidget::grabNormAreaCorner1FromMousePoint(int x, int y) {
  m_params.normalizationRegion.first = Mantid::Kernel::V2D(x, y);
  m_ui.spinBox_norm_top_x->setValue(x);
  m_ui.spinBox_norm_top_y->setValue(y);
}

void ImageROIViewQtWidget::grabNormAreaCorner2FromMousePoint(int x, int y) {
  m_params.normalizationRegion.second = Mantid::Kernel::V2D(x, y);
  m_ui.spinBox_norm_bottom_x->setValue(x);
  m_ui.spinBox_norm_bottom_y->setValue(y);
}

/**
 * This is an update and implicity a finish, as there's only one
 * update for the CoR (single point-click). The coordinates count as
 * usual in Qt widgets. Top-left is (0,0).
 *
 * @param x position on x axis (local to the image)
 * @param y position on y axis (local to the image)
 */
void ImageROIViewQtWidget::mouseUpdateCoR(int x, int y) {
  grabCoRFromMousePoint(x, y);
  refreshROIetAl();

  m_presenter->notify(IImageROIPresenter::FinishedCoR);
}

/**
 * Start of ROI selection (or first click after pushing "select
 * ROI". The rectangle starts as a point from the mouse click.
 *
 * @param x position on x axis (local to the image)
 * @param y position on y axis (local to the image)
 */
void ImageROIViewQtWidget::mouseUpdateROICorners12(int x, int y) {
  grabROICorner1FromMousePoint(x, y);
  grabROICorner2FromMousePoint(x, y);
  refreshROIetAl();
  m_selectionState = IImageROIView::SelectROISecond;
}

/**
 * Change the rectangle while pressing the mouse button. The first
 * corner stays at the first click, now only the second corner changes
 * to the new mouse position. On release of the mouse button we'll get
 * to mouseFinishROICorner2() and end the selection of the ROI.
 *
 * @param x position on x axis (local to the image)
 * @param y position on y axis (local to the image)
 */
void ImageROIViewQtWidget::mouseUpdateROICorner2(int x, int y) {
  grabROICorner2FromMousePoint(x, y);
  refreshROIetAl();
}

/**
 * End of ROI selection (or mouse button release after clicking once
 * and move, all after pushing "select ROI". The second corner of the
 * rectangle is set at the current position.
 *
 * @param x position on x axis (local to the image)
 * @param y position on y axis (local to the image)
 */
void ImageROIViewQtWidget::mouseFinishROI(int x, int y) {
  grabROICorner2FromMousePoint(x, y);
  refreshROIetAl();
  m_presenter->notify(IImageROIPresenter::FinishedROI);
}

void ImageROIViewQtWidget::mouseUpdateNormAreaCorners12(int x, int y) {
  grabNormAreaCorner1FromMousePoint(x, y);
  grabNormAreaCorner2FromMousePoint(x, y);
  refreshROIetAl();
  m_selectionState = IImageROIView::SelectNormAreaSecond;
}

void ImageROIViewQtWidget::mouseUpdateNormAreaCorner2(int x, int y) {
  grabNormAreaCorner2FromMousePoint(x, y);
  refreshROIetAl();
}

void ImageROIViewQtWidget::mouseFinishNormArea(int x, int y) {
  grabNormAreaCorner2FromMousePoint(x, y);
  refreshROIetAl();
  m_presenter->notify(IImageROIPresenter::FinishedNormalization);
}

void ImageROIViewQtWidget::readSettings() {
  QSettings qs;
  qs.beginGroup(QString::fromStdString(m_settingsGroup));
  restoreGeometry(qs.value("interface-win-geometry").toByteArray());
  qs.endGroup();
}

void ImageROIViewQtWidget::closeEvent(QCloseEvent *event) {
  m_presenter->notify(IImageROIPresenter::ShutDown);
  event->accept();
}

} // namespace CustomInterfaces
} // namespace MantidQt
