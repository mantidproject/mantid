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
#include <QTimer>

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
  //    we have a first working version of the "lean MD workspace". This method
  //    would then load into one workspace of such type.
  // b) load as workspace group - this is done in the overloaded method below

  // enableParamWidgets(true);
}

void ImageROIViewQtWidget::showStack(
    const Mantid::API::WorkspaceGroup_sptr &wsg,
    const Mantid::API::WorkspaceGroup_sptr &wsgFlats,
    const Mantid::API::WorkspaceGroup_sptr &wsgDarks) {
  if (0 == wsg->size())
    return;

  m_stackSamples = wsg;
  m_stackFlats = wsgFlats;
  m_stackDarks = wsgDarks;

  resetWidgetsOnNewStack();

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

  showProjection(m_stackSamples, 0);
  initParamWidgets(width, height);
  refreshROIetAl();
  enableParamWidgets(true);
  enableImageTypes(wsg && wsg->size() > 0, wsgFlats && wsgFlats->size() > 0,
                   wsgDarks && wsgDarks->size() > 0);
}

void ImageROIViewQtWidget::showProjection(
    const Mantid::API::WorkspaceGroup_sptr &wsg, size_t idx) {

  m_params.rotation = 0;
  m_ui.comboBox_rotation->setCurrentIndex(0);
  showProjectionImage(wsg, idx, 0);

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

void ImageROIViewQtWidget::enableActions(bool enable) {
  m_ui.pushButton_browse_img->setEnabled(enable);
  m_ui.pushButton_play->setEnabled(enable);
  m_ui.comboBox_image_type->setEnabled(enable);
  m_ui.comboBox_rotation->setEnabled(enable);
  m_ui.groupBox_cor->setEnabled(enable);
  m_ui.groupBox_roi->setEnabled(enable);
  m_ui.groupBox_norm->setEnabled(enable);
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

void ImageROIViewQtWidget::enableImageTypes(bool enableSamples,
                                            bool enableFlats,
                                            bool enableDarks) {
  QComboBox *itypes = m_ui.comboBox_image_type;
  if (!itypes || 3 != itypes->count())
    return;

  std::vector<bool> enable = {enableSamples, enableFlats, enableDarks};
  for (size_t idx = 0; idx < enable.size(); idx++) {
    QVariant var;
    if (!enable[idx]) {
      var = 0;
    } else {
      var = 1 | 32;
    }
    // trick to display a combobox entry as a disabled/enabled row
    QModelIndex modelIdx = itypes->model()->index(static_cast<int>(idx), 0);
    itypes->model()->setData(modelIdx, var, Qt::UserRole - 1);
  }
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

  m_ui.comboBox_image_type->setCurrentIndex(0);
  m_ui.comboBox_rotation->setCurrentIndex(0);

  sizes.clear();
  sizes.push_back(10);
  sizes.push_back(1000);
  m_ui.splitter_img_vertical->setSizes(sizes);
  m_ui.horizontalScrollBar_img_stack->setEnabled(false);
  m_ui.lineEdit_img_seq->setText("---");
  m_ui.label_img_name->setText("none");

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

  // "Play" the stack for quick visualization of input images
  connect(m_ui.pushButton_play, SIGNAL(released()), this, SLOT(playClicked()));

  // image sequence scroll/slide:
  connect(m_ui.horizontalScrollBar_img_stack, SIGNAL(valueChanged(int)), this,
          SLOT(updateFromImagesSlider(int)));

  // image rotation
  connect(m_ui.comboBox_rotation, SIGNAL(currentIndexChanged(int)), this,
          SLOT(rotationUpdated(int)));

  // image type
  connect(m_ui.comboBox_image_type, SIGNAL(currentIndexChanged(int)), this,
          SLOT(imageTypeUpdated(int)));

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

void ImageROIViewQtWidget::resetWidgetsOnNewStack() {
  m_ui.horizontalScrollBar_img_stack->setEnabled(true);
  m_ui.horizontalScrollBar_img_stack->setMinimum(0);
  m_ui.horizontalScrollBar_img_stack->setMaximum(
      static_cast<int>(m_stackSamples->size() - 1));
  m_ui.comboBox_rotation->setCurrentIndex(0);
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

  drawCenterCrossSymbol(painter, m_params.cor);

  drawBoxROI(painter, m_params.roi.first, m_params.roi.second);

  drawBoxNormalizationRegion(painter, m_params.normalizationRegion.first,
                             m_params.normalizationRegion.second);

  m_ui.label_img->setPixmap(toDisplay);
}

void ImageROIViewQtWidget::refreshCoR() {
  const QPixmap *pp = m_ui.label_img->pixmap();
  if (!pp)
    return;

  grabCoRFromWidgets();

  QPixmap toDisplay(*m_basePixmap.get());
  QPainter painter(&toDisplay);
  drawCenterCrossSymbol(painter, m_params.cor);

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
  drawBoxROI(painter, m_params.roi.first, m_params.roi.second);

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

  drawBoxNormalizationRegion(painter, m_params.normalizationRegion.first,
                             m_params.normalizationRegion.second);

  m_ui.label_img->setPixmap(toDisplay);
}

void ImageROIViewQtWidget::drawCenterCrossSymbol(QPainter &painter,
                                                 Mantid::Kernel::V2D &center) {
  // TODO: display settings / nicer symbol?

  QPen penCoR(Qt::red);
  painter.setPen(penCoR);
  painter.drawLine(
      static_cast<int>(center.X() - 5), static_cast<int>(center.Y()),
      static_cast<int>(center.X() + 5), static_cast<int>(center.Y()));
  painter.drawLine(
      static_cast<int>(center.X()), static_cast<int>(center.Y() - 5),
      static_cast<int>(center.X()), static_cast<int>(center.Y() + 5));
}

void ImageROIViewQtWidget::drawBoxROI(QPainter &painter,
                                      Mantid::Kernel::V2D &first,
                                      Mantid::Kernel::V2D &second) {
  QPen penROI(Qt::green);
  painter.setPen(penROI);
  painter.drawRect(static_cast<int>(first.X()), static_cast<int>(first.Y()),
                   static_cast<int>(second.X() - first.X()),
                   static_cast<int>(second.Y() - first.Y()));
}

void ImageROIViewQtWidget::drawBoxNormalizationRegion(
    QPainter &painter, Mantid::Kernel::V2D &first,
    Mantid::Kernel::V2D &second) {
  QPen penNA(Qt::yellow);
  painter.setPen(penNA);
  painter.drawRect(static_cast<int>(first.X()), static_cast<int>(first.Y()),
                   static_cast<int>(second.X() - first.X()),
                   static_cast<int>(second.Y() - first.Y()));
}

void ImageROIViewQtWidget::enableParamWidgets(bool enable) {
  m_ui.comboBox_image_type->setEnabled(enable);
  m_ui.comboBox_rotation->setEnabled(enable);

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
  if (params.rotation > 0 && params.rotation <= 360) {
    m_ui.comboBox_rotation->setCurrentIndex(
        static_cast<int>(params.rotation / 90.0f) % 4);
  } else {
    m_ui.comboBox_rotation->setCurrentIndex(0);
  }

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

void ImageROIViewQtWidget::playClicked() {
  m_presenter->notify(IImageROIPresenter::PlayStartStop);
}

void ImageROIViewQtWidget::playStart() {
  m_ui.pushButton_play->setText("Stop");
  // start timer
  m_playTimer = Mantid::Kernel::make_unique<QTimer>(this);
  connect(m_playTimer.get(), SIGNAL(timeout()), this, SLOT(updatePlay()));
  m_playTimer->start(133);
  m_ui.pushButton_play->setEnabled(true);
}

void ImageROIViewQtWidget::playStop() {
  // stop timer
  m_playTimer->stop();
  m_ui.pushButton_play->setText("Play");
}

void ImageROIViewQtWidget::updatePlay() {
  // TODO: maybe change to sample? Not obvious what would be users'
  // preference
  int val = m_ui.horizontalScrollBar_img_stack->value();
  ++val;
  if (m_ui.horizontalScrollBar_img_stack->maximum() <= val) {
    val = m_ui.horizontalScrollBar_img_stack->minimum();
  }
  m_ui.horizontalScrollBar_img_stack->setValue(val);
  showProjectionImage(m_stackSamples, val, currentRotationAngle());
}

void ImageROIViewQtWidget::browseImgClicked() {
  m_presenter->notify(IImageROIPresenter::BrowseImgOrStack);
}

void ImageROIViewQtWidget::updateFromImagesSlider(int /* current */) {
  m_presenter->notify(IImageROIPresenter::UpdateImgIndex);
}

void ImageROIViewQtWidget::imageTypeUpdated(int /* idx */) {
  m_presenter->notify(ImageROIPresenter::ChangeImageType);
}

void ImageROIViewQtWidget::rotationUpdated(int /* idx */) {
  m_params.rotation =
      static_cast<float>(m_ui.comboBox_rotation->currentIndex()) * 90.0f;
  m_presenter->notify(ImageROIPresenter::ChangeRotation);
}

size_t ImageROIViewQtWidget::currentImgIndex() const {
  return m_ui.horizontalScrollBar_img_stack->value();
}

void ImageROIViewQtWidget::updateImgWithIndex(size_t idx) {
  int max = m_ui.horizontalScrollBar_img_stack->maximum();
  int current = m_ui.horizontalScrollBar_img_stack->value();

  m_ui.lineEdit_img_seq->setText(
      QString::fromStdString(boost::lexical_cast<std::string>(current + 1) +
                             "/" + boost::lexical_cast<std::string>(max + 1)));

  showProjectionImage(currentImageTypeStack(), idx, currentRotationAngle());
}

void ImageROIViewQtWidget::showProjectionImage(
    const Mantid::API::WorkspaceGroup_sptr &wsg, size_t idx,
    float rotationAngle) {

  size_t width, height;
  std::string imgName;
  Mantid::API::MatrixWorkspace_sptr ws;
  try {
    checkNewProjectionImage(wsg, idx, width, height, ws, imgName);
  } catch (std::runtime_error &rexc) {
    QMessageBox::critical(this, "Cannot load image",
                          "There was a problem while trying to "
                          "find essential parameters of the image. Details: " +
                              QString::fromStdString(rexc.what()));
    return;
  }

  m_ui.label_img_name->setText(QString::fromStdString(imgName));
  // load / transfer image into a QImage, handling the rotation, width height
  // consistently
  QPixmap pixmap = transferWSImageToQPixmap(ws, width, height, rotationAngle);

  m_ui.label_img->setPixmap(pixmap);
  m_ui.label_img->show();
  m_basePixmap.reset(new QPixmap(pixmap));

  refreshROIetAl();
}

/**
 * Paint an image from a workspace (MatrixWorkspace) into a
 * QPixmap. The QPixmap can then be display by for example setting
 * it into a QLabel widget.
 *
 * @param ws image workspace
 *
 * @param width width of the image in pixels
 *
 * @param height height of the image in pixels
 *
 * @param rotationAngle rotate the image by this angle with respect to the
 *  original image in the workspace when displaying it
 */
QPixmap
ImageROIViewQtWidget::transferWSImageToQPixmap(const MatrixWorkspace_sptr ws,
                                               size_t width, size_t height,
                                               float rotationAngle) {

  // find min and max to scale pixel values
  double min = std::numeric_limits<double>::max(),
         max = std::numeric_limits<double>::min();
  getPixelMinMax(ws, min, max);
  if (max <= min) {
    QMessageBox::warning(
        this, "Empty image!",
        "The image could be loaded but it contains "
        "effectively no information, all pixels have the same value.");
    // black picture
    QPixmap pix(static_cast<int>(width), static_cast<int>(height));
    pix.fill(QColor(0, 0, 0));
    return pix;
  }

  QImage rawImg(QSize(static_cast<int>(width), static_cast<int>(height)),
                QImage::Format_RGB32);
  const double max_min = max - min;
  const double scaleFactor = 255.0 / max_min;

  for (size_t yi = 0; yi < width; ++yi) {
    const auto &yvec = ws->readY(yi);
    for (size_t xi = 0; xi < width; ++xi) {
      const double &v = yvec[xi];
      // color the range min-max in gray scale. To apply different color
      // maps you'd need to use rawImg.setColorTable() or similar.
      const int scaled = static_cast<int>(scaleFactor * (v - min));
      QRgb vRgb = qRgb(scaled, scaled, scaled);

      size_t rotX = 0, rotY = 0;
      if (0 == rotationAngle) {
        rotX = xi;
        rotY = yi;
      } else if (90 == rotationAngle) {
        rotX = height - (yi + 1);
        rotY = xi;
      } else if (180 == rotationAngle) {
        rotX = width - (xi + 1);
        rotY = height - (yi + 1);
      } else if (270 == rotationAngle) {
        rotX = yi;
        rotY = width - (xi + 1);
      }
      rawImg.setPixel(static_cast<int>(rotX), static_cast<int>(rotY), vRgb);
    }
  }

  // paint image direct from QImage object
  QPixmap pix = QPixmap::fromImage(rawImg);
  // Alternative, drawing with a painter:
  // QPixmap pix(static_cast<int>(width), static_cast<int>(height));
  // QPainter painter;
  // painter.begin(&pix);
  // painter.drawImage(0, 0, rawImg);
  // painter.end();

  return pix;
}

/**
 * Finds several parameters from an image in a stack (given the group
 * of images and an index), checking for errors. This is meant to be
 * used whenever a new image is going to be shown (for example when
 * using the slider to go through a stack).
 *
 * @param wsg the group of images
 * @param idx index of the image
 * @param width width or number of columns (in pixels)
 * @param height height or number of rows (in pixels)
 * @param imgWS image workspace corresponding to the index given
 * @param imgName name of the image (normally derived from the
 * original filename)
 *
 * @throws runtime_error when unrecoverable errors are found (no
 * data, issue with dimensions, etc.).
 */
void ImageROIViewQtWidget::checkNewProjectionImage(
    const Mantid::API::WorkspaceGroup_sptr &wsg, size_t idx, size_t &width,
    size_t &height, Mantid::API::MatrixWorkspace_sptr &imgWS,
    std::string &imgName) {
  try {
    imgWS = boost::dynamic_pointer_cast<MatrixWorkspace>(wsg->getItem(idx));
    if (!imgWS)
      return;
  } catch (std::exception &exc) {
    throw std::runtime_error("Could not find the image data: " +
                             std::string(exc.what()));
  }

  try {
    getCheckedDimensions(imgWS, width, height);
  } catch (std::runtime_error &rexc) {
    throw std::runtime_error("Could not find the width of the image: " +
                             std::string(rexc.what()));
  }

  try {
    imgName = imgWS->run().getLogData("run_title")->value();
  } catch (std::exception &e) {
    QMessageBox::warning(
        this, "Cannot load image information",
        "There was a problem while "
        " trying to find the name (run_title log entry) of the image: " +
            QString::fromStdString(e.what()));
  }
}

/**
 * Gets the width and height of an image MatrixWorkspace.
 *
 * @param ws image workspace
 *
 * @param width width of the image in pixels (bins)
 *
 * @param height height of the image in pixels (histograms)
 *
 * @throws runtime_error if there are problems when retrieving the
 * width/height or if they are inconsistent.
 */
void ImageROIViewQtWidget::getCheckedDimensions(const MatrixWorkspace_sptr ws,
                                                size_t &width, size_t &height) {
  const size_t MAXDIM = 2048 * 32;
  const std::string widthLogName = "Axis1";
  try {
    width = boost::lexical_cast<size_t>(
        ws->run().getLogData(widthLogName)->value());
    // TODO: add a settings option for this (like max mem allocation for
    // images)?
    if (width >= MAXDIM)
      width = MAXDIM;
  } catch (std::exception &exc) {
    throw std::runtime_error(
        "Could not find the width of the image from the run log data (entry: " +
        widthLogName + "). Error: " + exc.what());
  }

  const std::string heightLogName = "Axis2";
  try {
    height = boost::lexical_cast<size_t>(
        ws->run().getLogData(heightLogName)->value());
    if (height >= MAXDIM)
      height = MAXDIM;
  } catch (std::exception &exc) {
    throw std::runtime_error("could not find the height of the image from the "
                             "run log data (entry: " +
                             heightLogName + "). Error: " + exc.what());
  }

  // images are loaded as 1 histogram == 1 row (1 bin per image column):
  size_t histos = ws->getNumberHistograms();
  size_t bins = ws->blocksize();
  if (height != histos || width != bins) {
    throw std::runtime_error(
        "Image dimensions do not match in the input image workspace. "
        "Could not load the expected "
        "number of rows and columns. The width and height found in the run log "
        "data of the input files is: " +
        std::to_string(width) + ", " + std::to_string(height) +
        ", but the number of bins, histograms in "
        "the workspace loaded is: " +
        std::to_string(histos) + ", " + std::to_string(bins) + ".");
  }
}

void ImageROIViewQtWidget::getPixelMinMax(MatrixWorkspace_sptr ws, double &min,
                                          double &max) {
  for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
    for (size_t j = 0; j < ws->blocksize(); ++j) {
      const double &v = ws->readY(i)[j];
      if (v < min)
        min = v;
      if (v > max)
        max = v;
    }
  }
}

float ImageROIViewQtWidget::currentRotationAngle() const {
  return m_params.rotation;
}

void ImageROIViewQtWidget::updateRotationAngle(float angle) {
  if (angle < 0 || (0 != static_cast<int>(angle) % 90))
    return;

  m_params.rotation = angle;
  m_ui.comboBox_rotation->setCurrentIndex(
      static_cast<int>((static_cast<int>(angle) / 90) % 4));

  showProjectionImage(currentImageTypeStack(), currentImgIndex(),
                      currentRotationAngle());
}

void ImageROIViewQtWidget::updateImageType(
    const Mantid::API::WorkspaceGroup_sptr wsg) {
  if (!wsg || 0 == wsg->size())
    return;

  const int newIdx = 0;
  m_ui.horizontalScrollBar_img_stack->setEnabled(true);
  m_ui.horizontalScrollBar_img_stack->setMinimum(0);
  m_ui.horizontalScrollBar_img_stack->setValue(newIdx);
  m_ui.horizontalScrollBar_img_stack->setMaximum(
      static_cast<int>(wsg->size() - 1));

  const size_t numPics = wsg->size();
  m_ui.lineEdit_img_seq->setText(
      QString::fromStdString("1/" + boost::lexical_cast<std::string>(numPics)));

  showProjectionImage(wsg, currentImgIndex(), currentRotationAngle());
}

Mantid::API::WorkspaceGroup_sptr
ImageROIViewQtWidget::currentImageTypeStack() const {
  int type = m_ui.comboBox_image_type->currentIndex();

  if (1 == type) {
    return m_stackFlats;
  } else if (2 == type) {
    return m_stackDarks;
  } else {
    return m_stackSamples;
  }
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
