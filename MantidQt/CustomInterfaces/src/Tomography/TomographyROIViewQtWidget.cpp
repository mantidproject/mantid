#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtAPI/MantidColorMap.h"

#include "MantidQtCustomInterfaces/Tomography/TomographyROIPresenter.h"
#include "MantidQtCustomInterfaces/Tomography/TomographyROIViewQtWidget.h"

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
const std::string TomographyROIViewQtWidget::m_settingsGroup =
    "CustomInterfaces/TomographyROIView";

TomographyROIViewQtWidget::TomographyROIViewQtWidget(QWidget *parent)
    : QWidget(parent), ITomographyROIView(), m_colorMapFilename(),
      m_imgWidth(0), m_imgHeight(0), m_selectionState(SelectNone),
      m_presenter(NULL) {
  initLayout();

  // using an event filter. might be worth refactoring into a specific
  // QLabel + selection of ROI+NormArea+CoR class
  // not using Qwt Pickers to avoid Qwt version issues..
  m_ui.label_img->installEventFilter(this);
}

TomographyROIViewQtWidget::~TomographyROIViewQtWidget() {
  m_presenter->notify(TomographyROIPresenter::ShutDown);
}

void TomographyROIViewQtWidget::setParams(ImageStackPreParams &params) {
  m_params = params;
  setParamWidgets(m_params);
}

void TomographyROIViewQtWidget::initLayout() {
  // setup container ui
  m_ui.setupUi(this);

  QList<int> sizes;
  sizes.push_back(1000);
  sizes.push_back(200);
  // between image and right panel
  m_ui.splitter_main_horiz->setSizes(sizes);

  sizes.clear();
  sizes.push_back(200);
  sizes.push_back(50);
  // between color bar and coordinates panel
  m_ui.splitter_right_horiz->setSizes(sizes);

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
  m_ui.colorBarWidget->setEnabled(false);

  m_ui.colorBarWidget->setViewRange(1, 65536);
  m_ui.colorBarWidget->setAutoScale(true);
  m_ui.colorBarWidget->setCheckBoxMode(
      MantidWidgets::ColorBarWidget::ADD_AUTOSCALE_ON_LOAD);
  m_ui.colorBarWidget->setAutoScaleTooltipText(
      "This flag signals that the color scale range\n"
      "will be set automatically to the current slice range\n"
      "when an image is loaded. Note that auto\n"
      "scaling will be applied when an image is loaded\n"
      "for the very first time.");

  readSettings();

  setupConnections();

  initParamWidgets(1, 1);
  grabCoRFromWidgets();
  grabROIFromWidgets();
  grabNormAreaFromWidgets();

  // presenter that knows how to handle a ITomographyROIView should take care
  // of all the logic. Note the view needs to now the concrete presenter here
  m_presenter.reset(new TomographyROIPresenter(this));

  // it will know what compute resources and tools we have available:
  // This view doesn't even know the names of compute resources, etc.
  m_presenter->notify(TomographyROIPresenter::Init);
}

void TomographyROIViewQtWidget::setupConnections() {

  // 'browse' buttons
  connect(m_ui.pushButton_browse_img, SIGNAL(released()), this,
          SLOT(browseImageClicked()));

  connect(m_ui.pushButton_browse_stack, SIGNAL(released()), this,
          SLOT(browseStackClicked()));

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

  // Clicking Auto COR
  connect(m_ui.pushButton_center_find, SIGNAL(released()), this,
          SLOT(findCORClicked()));

  // image sequence scroll/slide:
  connect(m_ui.horizontalScrollBar_img_stack, SIGNAL(valueChanged(int)), this,
          SLOT(updateFromImagesSlider(int)));

  // changing CoR tool
  connect(m_ui.comboBox_center_method, SIGNAL(currentIndexChanged(int)), this,
          SLOT(autoCoRToolChanged(int)));

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

  connect(m_ui.spinBox_roi_right, SIGNAL(valueChanged(int)), this,
          SLOT(valueUpdatedROI(int)));
  connect(m_ui.spinBox_roi_top, SIGNAL(valueChanged(int)), this,
          SLOT(valueUpdatedROI(int)));
  connect(m_ui.spinBox_roi_left, SIGNAL(valueChanged(int)), this,
          SLOT(valueUpdatedROI(int)));
  connect(m_ui.spinBox_roi_bottom, SIGNAL(valueChanged(int)), this,
          SLOT(valueUpdatedROI(int)));

  connect(m_ui.spinBox_norm_right, SIGNAL(valueChanged(int)), this,
          SLOT(valueUpdatedNormArea(int)));
  connect(m_ui.spinBox_norm_top, SIGNAL(valueChanged(int)), this,
          SLOT(valueUpdatedNormArea(int)));
  connect(m_ui.spinBox_norm_left, SIGNAL(valueChanged(int)), this,
          SLOT(valueUpdatedNormArea(int)));
  connect(m_ui.spinBox_norm_bottom, SIGNAL(valueChanged(int)), this,
          SLOT(valueUpdatedNormArea(int)));

  // colors
  connect(m_ui.colorBarWidget, SIGNAL(colorBarDoubleClicked()), this,
          SLOT(loadColorMapRequest()));

  connect(m_ui.colorBarWidget, SIGNAL(changedColorRange(double, double, bool)),
          this, SLOT(colorRangeChanged()));
}

void TomographyROIViewQtWidget::readSettings() {
  QSettings qs;
  qs.beginGroup(QString::fromStdString(m_settingsGroup));

  // Color bar widget settings
  // The factory default would be "Gray.map"
  const auto mapsDir = QString::fromStdString(
      Mantid::Kernel::ConfigService::Instance().getString(
          "colormaps.directory"));
  const auto defMapFile =
      QFileInfo(mapsDir).absoluteDir().filePath("Gamma1.map");
  const auto filepath = qs.value("colorbar-file", defMapFile).toString();
  m_ui.colorBarWidget->getColorMap().loadMap(filepath);
  m_ui.colorBarWidget->updateColorMap();

  m_ui.colorBarWidget->setScale(qs.value("colorbar-scale-type", 0).toInt());
  m_ui.colorBarWidget->setExponent(
      qs.value("colorbar-power-exponent", 2.0).toDouble());
  m_ui.colorBarWidget->setAutoScale(
      qs.value("colorbar-autoscale", false).toBool());
  // ColorBarWidget doesn't support setting "AutoColorScaleforCurrentSlice"
  // qsetting: "colorbar-autoscale-current-slice"

  restoreGeometry(qs.value("interface-win-geometry").toByteArray());
  qs.endGroup();
}

void TomographyROIViewQtWidget::saveSettings() const {
  QSettings qs;
  qs.beginGroup(QString::fromStdString(m_settingsGroup));

  // Color bar widget settings
  qs.setValue("colorbar-file",
              m_ui.colorBarWidget->getColorMap().getFilePath());
  qs.setValue("colorbar-scale-type", m_ui.colorBarWidget->getScale());
  qs.setValue("colorbar-power-exponent", m_ui.colorBarWidget->getExponent());
  qs.setValue("colorbar-autoscale", m_ui.colorBarWidget->getAutoScale());
  qs.setValue("colorbar-autoscale-current-slice",
              m_ui.colorBarWidget->getAutoScaleforCurrentSlice());

  qs.setValue("interface-win-geometry", saveGeometry());
  qs.endGroup();
}

ImageStackPreParams TomographyROIViewQtWidget::userSelection() const {
  return m_params;
}

void TomographyROIViewQtWidget::changeSelectionState(
    const ITomographyROIView::SelectionState &state) {
  m_selectionState = state;
}

void TomographyROIViewQtWidget::showStack(const std::string & /*path*/) {
  // TODO:
  // a) load as proper stack of images workspace - this can only be done when
  //    we have a first working version of the "lean MD workspace". This method
  //    would then load into one workspace of such type.
  // b) load as workspace group - this is done in the overloaded method below

  // enableParamWidgets(true);
}

void TomographyROIViewQtWidget::showStack(
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
  refreshImage();
  enableParamWidgets(true);
  enableImageTypes(wsg && wsg->size() > 0, wsgFlats && wsgFlats->size() > 0,
                   wsgDarks && wsgDarks->size() > 0);
}

void TomographyROIViewQtWidget::showProjection(
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

void TomographyROIViewQtWidget::userWarning(const std::string &err,
                                            const std::string &description) {
  QMessageBox::warning(this, QString::fromStdString(err),
                       QString::fromStdString(description), QMessageBox::Ok,
                       QMessageBox::Ok);
}

void TomographyROIViewQtWidget::userError(const std::string &err,
                                          const std::string &description) {
  QMessageBox::critical(this, QString::fromStdString(err),
                        QString::fromStdString(description), QMessageBox::Ok,
                        QMessageBox::Ok);
}

void TomographyROIViewQtWidget::enableActions(bool enable) {
  m_ui.pushButton_browse_img->setEnabled(enable);
  m_ui.pushButton_browse_stack->setEnabled(enable);
  m_ui.pushButton_play->setEnabled(enable);
  m_ui.pushButton_center_find->setEnabled(enable);

  m_ui.colorBarWidget->setEnabled(enable);

  m_ui.comboBox_center_method->setEnabled(enable);
  m_ui.comboBox_image_type->setEnabled(enable);
  m_ui.comboBox_rotation->setEnabled(enable);
  m_ui.groupBox_cor->setEnabled(enable);
  m_ui.groupBox_roi->setEnabled(enable);
  m_ui.groupBox_norm->setEnabled(enable);
}

std::string TomographyROIViewQtWidget::askImgOrStackPath() {
  // get path
  QString prevPath =
      MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  QString path(QFileDialog::getExistingDirectory(
      this, tr("Open a stack of images (directory containing sample, dark and "
               "flat images, or a directory containing images)"),
      prevPath));
  if (!path.isEmpty()) {
    MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(path);
  }

  return path.toStdString();
}

std::string TomographyROIViewQtWidget::askSingleImagePath() {
  // get path
  // QString fitsStr = QString("Supported formats: FITS, TIFF and PNG "
  //                           "(*.fits *.fit *.tiff *.tif *.png);;"
  //                           "FITS, Flexible Image Transport System images "
  //                           "(*.fits *.fit);;"
  //                           "TIFF, Tagged Image File Format "
  //                           "(*.tif *.tiff);;"
  //                           "PNG, Portable Network Graphics "
  //                           "(*.png);;"
  //                           "Other extensions/all files (*)");
  // only FITS is supported right now
  QString fitsStr = QString("Supported formats: FITS"
                            "(*.fits *.fit);;"
                            "FITS, Flexible Image Transport System images "
                            "(*.fits *.fit);;");
  QString prevPath =
      MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  QString filepath(
      QFileDialog::getOpenFileName(this, tr("Open image"), prevPath, fitsStr));
  if (!filepath.isEmpty()) {
    MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(
        filepath);
  }

  return filepath.toStdString();
}

void TomographyROIViewQtWidget::enableImageTypes(bool enableSamples,
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

void TomographyROIViewQtWidget::resetCoR() {
  int midx =
      (m_ui.spinBox_cor_x->minimum() + m_ui.spinBox_cor_x->maximum()) / 2;
  m_ui.spinBox_cor_x->setValue(midx);
  int midy =
      (m_ui.spinBox_cor_y->minimum() + m_ui.spinBox_cor_y->maximum()) / 2;
  m_ui.spinBox_cor_y->setValue(midy);
}

void TomographyROIViewQtWidget::resetROI() {
  m_ui.spinBox_roi_left->setValue(0);
  m_ui.spinBox_roi_top->setValue(0);
  m_ui.spinBox_roi_right->setValue(m_ui.spinBox_roi_right->maximum());
  m_ui.spinBox_roi_bottom->setValue(m_ui.spinBox_roi_bottom->maximum());
}

void TomographyROIViewQtWidget::resetNormArea() {
  m_ui.spinBox_norm_right->setValue(0);
  m_ui.spinBox_norm_top->setValue(0);
  m_ui.spinBox_norm_left->setValue(0);
  m_ui.spinBox_norm_bottom->setValue(0);
}

void TomographyROIViewQtWidget::resetWidgetsOnNewStack() {
  m_ui.horizontalScrollBar_img_stack->setEnabled(true);
  m_ui.horizontalScrollBar_img_stack->setMinimum(0);
  m_ui.horizontalScrollBar_img_stack->setMaximum(
      static_cast<int>(m_stackSamples->size() - 1));
  m_ui.comboBox_rotation->setCurrentIndex(0);
}

void TomographyROIViewQtWidget::valueUpdatedCoR(int) {
  grabCoRFromWidgets();
  refreshImage();
}

void TomographyROIViewQtWidget::valueUpdatedROI(int) {
  grabROIFromWidgets();
  refreshImage();
}

void TomographyROIViewQtWidget::valueUpdatedNormArea(int) {
  grabNormAreaFromWidgets();
  refreshImage();
}

/**
 * Parameter values from spin box widgets => coordinate parameters
 * data member
 */
void TomographyROIViewQtWidget::grabCoRFromWidgets() {
  m_params.cor = Mantid::Kernel::V2D(m_ui.spinBox_cor_x->value(),
                                     m_ui.spinBox_cor_y->value());
}

void TomographyROIViewQtWidget::grabROIFromWidgets() {
  m_params.roi =
      std::make_pair(Mantid::Kernel::V2D(m_ui.spinBox_roi_right->value(),
                                         m_ui.spinBox_roi_top->value()),
                     Mantid::Kernel::V2D(m_ui.spinBox_roi_left->value(),
                                         m_ui.spinBox_roi_bottom->value()));
}

void TomographyROIViewQtWidget::grabNormAreaFromWidgets() {
  m_params.normalizationRegion =
      std::make_pair(Mantid::Kernel::V2D(m_ui.spinBox_norm_right->value(),
                                         m_ui.spinBox_norm_top->value()),
                     Mantid::Kernel::V2D(m_ui.spinBox_norm_left->value(),
                                         m_ui.spinBox_norm_bottom->value()));
}

/**
 * Updates the image view with the current image index and selection
 * of parameters (ROI, normalization area, CoR). This needs to be used
 * for every event that modifies the current image and/or selection of
 * parameters.
 */
void TomographyROIViewQtWidget::refreshImage() {

  const QPixmap *pp = m_ui.label_img->pixmap();
  if (!pp)
    return;

  m_ui.label_img->setMaximumWidth(static_cast<int>(m_imgWidth));
  m_ui.label_img->setMaximumHeight(static_cast<int>(m_imgHeight));

  QPixmap toDisplay(*m_basePixmap.get());
  QPainter painter(&toDisplay);

  drawCenterCrossSymbol(painter, m_params.cor);

  drawBoxROI(painter, m_params.roi.first, m_params.roi.second);

  drawBoxNormalizationRegion(painter, m_params.normalizationRegion.first,
                             m_params.normalizationRegion.second);

  m_ui.label_img->setPixmap(toDisplay);
}

void TomographyROIViewQtWidget::refreshCoR() {
  const QPixmap *pp = m_ui.label_img->pixmap();
  if (!pp)
    return;

  grabCoRFromWidgets();

  QPixmap toDisplay(*m_basePixmap.get());
  QPainter painter(&toDisplay);
  drawCenterCrossSymbol(painter, m_params.cor);

  m_ui.label_img->setPixmap(toDisplay);
}

void TomographyROIViewQtWidget::refreshROI() {
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

void TomographyROIViewQtWidget::refreshNormArea() {
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

void TomographyROIViewQtWidget::drawCenterCrossSymbol(
    QPainter &painter, const Mantid::Kernel::V2D &center) const {
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

void TomographyROIViewQtWidget::drawBoxROI(
    QPainter &painter, const Mantid::Kernel::V2D &first,
    const Mantid::Kernel::V2D &second) const {
  QPen penROI(Qt::green);
  painter.setPen(penROI);
  painter.drawRect(static_cast<int>(first.X()), static_cast<int>(first.Y()),
                   static_cast<int>(second.X() - first.X()),
                   static_cast<int>(second.Y() - first.Y()));
}

void TomographyROIViewQtWidget::drawBoxNormalizationRegion(
    QPainter &painter, const Mantid::Kernel::V2D &first,
    const Mantid::Kernel::V2D &second) const {
  QPen penNA(Qt::yellow);
  painter.setPen(penNA);
  painter.drawRect(static_cast<int>(first.X()), static_cast<int>(first.Y()),
                   static_cast<int>(second.X() - first.X()),
                   static_cast<int>(second.Y() - first.Y()));
}

void TomographyROIViewQtWidget::enableParamWidgets(bool enable) {
  m_ui.comboBox_image_type->setEnabled(enable);
  m_ui.comboBox_rotation->setEnabled(enable);

  m_ui.groupBox_cor->setEnabled(enable);
  m_ui.groupBox_roi->setEnabled(enable);
  m_ui.groupBox_norm->setEnabled(enable);
}

void TomographyROIViewQtWidget::initParamWidgets(size_t maxWidth,
                                                 size_t maxHeight) {
  m_imgWidth = static_cast<int>(maxWidth);
  m_imgHeight = static_cast<int>(maxHeight);

  m_ui.spinBox_cor_x->setMinimum(0);
  m_ui.spinBox_cor_x->setMaximum(m_imgWidth - 1);
  m_ui.spinBox_cor_y->setMinimum(0);
  m_ui.spinBox_cor_y->setMaximum(m_imgHeight - 1);
  resetCoR();

  m_ui.spinBox_roi_right->setMinimum(0);
  m_ui.spinBox_roi_right->setMaximum(m_imgWidth - 1);
  m_ui.spinBox_roi_top->setMinimum(0);
  m_ui.spinBox_roi_top->setMaximum(m_imgHeight - 1);

  m_ui.spinBox_roi_left->setMinimum(0);
  m_ui.spinBox_roi_left->setMaximum(m_imgWidth - 1);
  m_ui.spinBox_roi_bottom->setMinimum(0);
  m_ui.spinBox_roi_bottom->setMaximum(m_imgHeight - 1);

  resetROI();

  m_ui.spinBox_norm_right->setMinimum(0);
  m_ui.spinBox_norm_right->setMaximum(m_imgWidth - 1);
  m_ui.spinBox_norm_top->setMinimum(0);
  m_ui.spinBox_norm_top->setMaximum(m_imgHeight - 1);

  m_ui.spinBox_norm_left->setMinimum(0);
  m_ui.spinBox_norm_left->setMaximum(m_imgWidth - 1);
  m_ui.spinBox_norm_bottom->setMinimum(0);
  m_ui.spinBox_norm_bottom->setMaximum(m_imgHeight - 1);

  resetNormArea();
}

void TomographyROIViewQtWidget::setParamWidgets(ImageStackPreParams &params) {
  if (params.rotation > 0 && params.rotation <= 360) {
    m_ui.comboBox_rotation->setCurrentIndex(
        static_cast<int>(params.rotation / 90.0f) % 4);
  } else {
    m_ui.comboBox_rotation->setCurrentIndex(0);
  }

  m_ui.spinBox_cor_x->setValue(static_cast<int>(params.cor.X()));
  m_ui.spinBox_cor_y->setValue(static_cast<int>(params.cor.Y()));

  m_ui.spinBox_roi_right->setValue(static_cast<int>(params.roi.first.X()));
  m_ui.spinBox_roi_top->setValue(static_cast<int>(params.roi.first.Y()));

  m_ui.spinBox_roi_left->setValue(static_cast<int>(params.roi.second.X()));
  m_ui.spinBox_roi_bottom->setValue(static_cast<int>(params.roi.second.Y()));

  m_ui.spinBox_norm_right->setValue(
      static_cast<int>(params.normalizationRegion.first.X()));
  m_ui.spinBox_norm_top->setValue(
      static_cast<int>(params.normalizationRegion.first.Y()));

  m_ui.spinBox_norm_left->setValue(
      static_cast<int>(params.normalizationRegion.second.X()));
  m_ui.spinBox_norm_bottom->setValue(
      static_cast<int>(params.normalizationRegion.second.Y()));
}

void TomographyROIViewQtWidget::corClicked() {
  m_presenter->notify(ITomographyROIPresenter::SelectCoR);
}

void TomographyROIViewQtWidget::corResetClicked() {
  m_presenter->notify(ITomographyROIPresenter::ResetCoR);
  refreshImage();
}

void TomographyROIViewQtWidget::roiClicked() {
  m_presenter->notify(ITomographyROIPresenter::SelectROI);
}

void TomographyROIViewQtWidget::roiResetClicked() {
  m_presenter->notify(ITomographyROIPresenter::ResetROI);
  refreshImage();
}

void TomographyROIViewQtWidget::normAreaClicked() {
  m_presenter->notify(ITomographyROIPresenter::SelectNormalization);
}

void TomographyROIViewQtWidget::normAreaResetClicked() {
  m_presenter->notify(ITomographyROIPresenter::ResetNormalization);
  refreshImage();
}

void TomographyROIViewQtWidget::playClicked() {
  m_presenter->notify(ITomographyROIPresenter::PlayStartStop);
}

void TomographyROIViewQtWidget::playStart() {
  m_ui.pushButton_play->setText("Stop");
  // start timer
  m_playTimer = Mantid::Kernel::make_unique<QTimer>(this);
  connect(m_playTimer.get(), SIGNAL(timeout()), this, SLOT(updatePlay()));
  m_playTimer->start(133);
  m_ui.pushButton_play->setEnabled(true);
}

void TomographyROIViewQtWidget::playStop() {
  // stop timer
  m_playTimer->stop();
  m_ui.pushButton_play->setText("Play");
}

void TomographyROIViewQtWidget::updatePlay() {
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

void TomographyROIViewQtWidget::browseStackClicked() {
  m_presenter->notify(ITomographyROIPresenter::BrowseStack);
}

void TomographyROIViewQtWidget::browseImageClicked() {
  m_presenter->notify(ITomographyROIPresenter::BrowseImage);
}

void TomographyROIViewQtWidget::updateFromImagesSlider(int /* current */) {
  m_presenter->notify(ITomographyROIPresenter::UpdateImgIndex);
}

void TomographyROIViewQtWidget::imageTypeUpdated(int /* idx */) {
  m_presenter->notify(TomographyROIPresenter::ChangeImageType);
}

void TomographyROIViewQtWidget::rotationUpdated(int /* idx */) {
  m_params.rotation =
      static_cast<float>(m_ui.comboBox_rotation->currentIndex()) * 90.0f;
  m_presenter->notify(TomographyROIPresenter::ChangeRotation);
}

void TomographyROIViewQtWidget::autoCoRToolChanged(int /* idx */) {
  // todo notify presenter that the tool is changed, however for now we only
  // have one tool -> tomopy
}

size_t TomographyROIViewQtWidget::currentImgIndex() const {
  return m_ui.horizontalScrollBar_img_stack->value();
}

void TomographyROIViewQtWidget::updateImgWithIndex(size_t idx) {
  int max = m_ui.horizontalScrollBar_img_stack->maximum();
  int current = m_ui.horizontalScrollBar_img_stack->value();

  m_ui.lineEdit_img_seq->setText(
      QString::fromStdString(boost::lexical_cast<std::string>(current + 1) +
                             "/" + boost::lexical_cast<std::string>(max + 1)));

  showProjectionImage(currentImageTypeStack(), idx, currentRotationAngle());
}

void TomographyROIViewQtWidget::showProjectionImage(
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

  refreshImage();
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
QPixmap TomographyROIViewQtWidget::transferWSImageToQPixmap(
    const MatrixWorkspace_sptr ws, size_t width, size_t height,
    float rotationAngle) {

  QImage rawImg(QSize(static_cast<int>(width), static_cast<int>(height)),
                QImage::Format_RGB32);

  double minVal = std::numeric_limits<double>::max(),
         maxVal = std::numeric_limits<double>::min();
  // find min and max to scale pixel values, whether from user-given
  // range, or auto-range
  if (!m_ui.colorBarWidget->getAutoScale()) {
    minVal = m_ui.colorBarWidget->getMinimum();
    maxVal = m_ui.colorBarWidget->getMaximum();
  } else {

    getPixelMinMax(ws, minVal, maxVal);
    if (maxVal <= minVal) {
      QMessageBox::warning(
          this, "Empty image!",
          "The image could be loaded but it contains "
          "effectively no information, all pixels have the same value.");
      // black picture
      QPixmap pix(static_cast<int>(width), static_cast<int>(height));
      pix.fill(QColor(0, 0, 0));
      return pix;
    }
    m_ui.colorBarWidget->setViewRange(minVal, maxVal);
  }
  const double max_min = maxVal - minVal;
  const double scaleFactor = 255.0 / max_min;

  // normal Qt rgb values
  QwtDoubleInterval rgbInterval(0.0, 255.0);
  const QVector<QRgb> colorTable =
      m_ui.colorBarWidget->getColorMap().colorTable(rgbInterval);
  rawImg.setColorTable(colorTable);

  for (size_t yi = 0; yi < width; ++yi) {
    const auto &yvec = ws->readY(yi);
    for (size_t xi = 0; xi < width; ++xi) {
      const double &v = yvec[xi];
      const int scaled = static_cast<int>(scaleFactor * (v - minVal));
      // You would use this for a trivial mapping of valus to colors:
      // Coloring the range min-max in gray scale. But setColorTable()
      // is used to apply different color maps from files QRgb vRgb =
      // qRgb(scaled, scaled, scaled);
      // rawImg.setPixel(static_cast<int>(rotX), static_cast<int>(rotY),
      // scaled);
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

      const auto rgb =
          m_ui.colorBarWidget->getColorMap().rgb(rgbInterval, scaled);
      rawImg.setPixel(static_cast<int>(rotX), static_cast<int>(rotY), rgb);
      // This would be faster, using the look-up color table, but for unknown
      // reasons MantidColorMap::colorTable forces linear scale.
      // rawImg.setPixel(static_cast<int>(rotX), static_cast<int>(rotY),
      // colorTable[scaled]);
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
void TomographyROIViewQtWidget::checkNewProjectionImage(
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
void TomographyROIViewQtWidget::getCheckedDimensions(
    const MatrixWorkspace_sptr ws, size_t &width, size_t &height) {
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

void TomographyROIViewQtWidget::getPixelMinMax(MatrixWorkspace_sptr ws,
                                               double &min, double &max) {
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

float TomographyROIViewQtWidget::currentRotationAngle() const {
  return m_params.rotation;
}

void TomographyROIViewQtWidget::updateRotationAngle(float angle) {
  if (angle < 0 || (0 != static_cast<int>(angle) % 90))
    return;

  m_params.rotation = angle;
  m_ui.comboBox_rotation->setCurrentIndex(
      static_cast<int>((static_cast<int>(angle) / 90) % 4));

  showProjectionImage(currentImageTypeStack(), currentImgIndex(),
                      currentRotationAngle());
}

void TomographyROIViewQtWidget::updateImageType(
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
TomographyROIViewQtWidget::currentImageTypeStack() const {
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
 * of new notifications in ITomographyROIPresenter, and making at least all
 * the mouseUpdateCoR, mouseUpdateROICorners12, mouseXXX methods
 * public in this view interface. This can be considered at a later
 * stage.
 *
 * @param obj object concerned by the event
 * @param event event received (mouse click, release, move, etc.)
 **/
bool TomographyROIViewQtWidget::eventFilter(QObject *obj, QEvent *event) {
  // quick ignore
  if (ITomographyROIView::SelectNone == m_selectionState)
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

      if (ITomographyROIView::SelectCoR == m_selectionState) {
        mouseUpdateCoR(x, y);
      } else if (ITomographyROIView::SelectROIFirst == m_selectionState) {
        mouseUpdateROICornersStartSelection(x, y);
      } else if (ITomographyROIView::SelectNormAreaFirst == m_selectionState) {
        mouseUpdateNormAreaCornersStartSelection(x, y);
      }

    } else if (type == QEvent::MouseMove) {

      if (ITomographyROIView::SelectROISecond == m_selectionState) {
        mouseUpdateROICornerContinuedSelection(x, y);
      } else if (ITomographyROIView::SelectNormAreaSecond == m_selectionState) {
        mouseUpdateNormAreaCornerContinuedSelection(x, y);
      }
    } else if (type == QEvent::MouseButtonRelease) {

      if (ITomographyROIView::SelectROISecond == m_selectionState) {
        mouseFinishROI(x, y);
      } else if (ITomographyROIView::SelectNormAreaSecond == m_selectionState) {
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
void TomographyROIViewQtWidget::grabCoRFromMousePoint(const int x,
                                                      const int y) {
  // m_params.cor = Mantid::Kernel::V2D(x, y);
  m_ui.spinBox_cor_x->setValue(x);
  m_ui.spinBox_cor_y->setValue(y);
}

void TomographyROIViewQtWidget::grabROICorner1FromMousePoint(const int x,
                                                             const int y) {
  // this is re-used for Norm Region selection as you cannot be selecting both
  // at the same time
  m_startOfRectangle.right = x;
  m_startOfRectangle.top = y;

  m_ui.spinBox_roi_right->setValue(x);
  m_ui.spinBox_roi_top->setValue(y);
}

void TomographyROIViewQtWidget::grabROICorner2FromMousePoint(const int x,
                                                             const int y) {
  updateValuesForSpinBoxes(x, y, m_startOfRectangle, m_ui.spinBox_roi_left,
                           m_ui.spinBox_roi_top, m_ui.spinBox_roi_right,
                           m_ui.spinBox_roi_bottom);
}

void TomographyROIViewQtWidget::grabNormAreaCorner1FromMousePoint(const int x,
                                                                  const int y) {
  // this is re-used for Norm Region selection as you cannot be selecting both
  // at the same time
  m_startOfRectangle.right = x;
  m_startOfRectangle.top = y;

  m_ui.spinBox_norm_right->setValue(x);
  m_ui.spinBox_norm_top->setValue(y);
}

void TomographyROIViewQtWidget::grabNormAreaCorner2FromMousePoint(const int x,
                                                                  const int y) {

  updateValuesForSpinBoxes(x, y, m_startOfRectangle, m_ui.spinBox_norm_left,
                           m_ui.spinBox_norm_top, m_ui.spinBox_norm_right,
                           m_ui.spinBox_norm_bottom);
}

void TomographyROIViewQtWidget::updateValuesForSpinBoxes(
    const int x, const int y, const RectangleXY startPositions,
    QSpinBox *spinLeft, QSpinBox *spinTop, QSpinBox *spinRight,
    QSpinBox *spinBottom) {

  // put at the top so that it has chance to fire before a setValue event
  spinLeft->blockSignals(true);
  spinTop->blockSignals(true);
  spinRight->blockSignals(true);
  spinBottom->blockSignals(true);

  int left = x;
  int right = startPositions.right;
  int bottom = y;
  int top = startPositions.top;

  // left side is over the right one
  if (left > right) {
    std::swap(left, right);
  }

  if (top > bottom) {
    std::swap(top, bottom);
  }

  spinLeft->setValue(left);
  spinTop->setValue(top);
  spinRight->setValue(right);
  spinBottom->setValue(bottom);

  spinLeft->blockSignals(false);
  spinTop->blockSignals(false);
  spinRight->blockSignals(false);
  spinBottom->blockSignals(false);
}

/**
 * This is an update and implicity a finish, as there's only one
 * update for the CoR (single point-click). The coordinates count as
 * usual in Qt widgets. Top-left is (0,0).
 *
 * @param x position on x axis (local to the image)
 * @param y position on y axis (local to the image)
 */
void TomographyROIViewQtWidget::mouseUpdateCoR(const int x, const int y) {
  grabCoRFromMousePoint(x, y);
  refreshImage();

  m_presenter->notify(ITomographyROIPresenter::FinishedCoR);
}

/**
 * Start of ROI selection (or first click after pushing "select
 * ROI". The rectangle starts as a point from the mouse click.
 *
 * @param x position on x axis (local to the image)
 * @param y position on y axis (local to the image)
 */
void TomographyROIViewQtWidget::mouseUpdateROICornersStartSelection(
    const int x, const int y) {
  grabROICorner1FromMousePoint(x, y);
  grabROICorner2FromMousePoint(x, y);
  refreshImage();
  m_selectionState = ITomographyROIView::SelectROISecond;
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
void TomographyROIViewQtWidget::mouseUpdateROICornerContinuedSelection(
    const int x, const int y) {
  grabROICorner2FromMousePoint(x, y);
  grabROIFromWidgets();
  refreshImage();
}

/**
 * End of ROI selection (or mouse button release after clicking once
 * and move, all after pushing "select ROI". The second corner of the
 * rectangle is set at the current position.
 *
 * @param x position on x axis (local to the image)
 * @param y position on y axis (local to the image)
 */
void TomographyROIViewQtWidget::mouseFinishROI(const int x, const int y) {
  grabROICorner2FromMousePoint(x, y);
  refreshImage();
  m_presenter->notify(ITomographyROIPresenter::FinishedROI);
}

void TomographyROIViewQtWidget::mouseUpdateNormAreaCornersStartSelection(
    const int x, const int y) {
  grabNormAreaCorner1FromMousePoint(x, y);
  grabNormAreaCorner2FromMousePoint(x, y);
  refreshImage();
  m_selectionState = ITomographyROIView::SelectNormAreaSecond;
}

void TomographyROIViewQtWidget::mouseUpdateNormAreaCornerContinuedSelection(
    const int x, const int y) {
  grabNormAreaCorner2FromMousePoint(x, y);
  grabNormAreaFromWidgets();
  refreshImage();
}

void TomographyROIViewQtWidget::mouseFinishNormArea(const int x, const int y) {
  grabNormAreaCorner2FromMousePoint(x, y);
  refreshImage();
  m_presenter->notify(ITomographyROIPresenter::FinishedNormalization);
}

/**
 * Slot for the signal emitted by the color bar widget when the user
 * requests a new color map.
 */
void TomographyROIViewQtWidget::loadColorMapRequest() {
  m_presenter->notify(ITomographyROIPresenter::UpdateColorMap);
}

std::string TomographyROIViewQtWidget::askColorMapFile() {
  QString filename = MantidColorMap::loadMapDialog(
      QString::fromStdString(m_colorMapFilename), this);
  return filename.toStdString();
}

void TomographyROIViewQtWidget::updateColorMap(const std::string &filename) {
  // Load from file
  m_ui.colorBarWidget->getColorMap().loadMap(QString::fromStdString(filename));
  m_ui.colorBarWidget->updateColorMap();

  // if (!m_colorMapFilename.empty())
  //  updateColorMap(m_colorMapFilename);
  m_ui.colorBarWidget->setViewRange(1, 65536);

  m_presenter->notify(ITomographyROIPresenter::ColorRangeUpdated);
}

/**
 * Slot for the signal emitted by the color bar widget when there's an
 * update in the values
 */
void TomographyROIViewQtWidget::colorRangeChanged() {
  // the presenter should handle the image display update
  m_presenter->notify(ITomographyROIPresenter::ColorRangeUpdated);
}

void TomographyROIViewQtWidget::closeEvent(QCloseEvent *event) {
  m_presenter->notify(ITomographyROIPresenter::ShutDown);
  event->accept();
}

void TomographyROIViewQtWidget::findCORClicked() {
  // this should run a --find-cor, empty executable path string signifies that
  // the default external interpretor will be used
  emit(findCORClicked("", {"--find-cor"}));
}

void TomographyROIViewQtWidget::readCoRFromProcessOutput(const QString &str) {
  if (str.isEmpty()) {
    // the process string is empty, it is likely the process crashed or was
    // unable to execute properly
    return;
  }

  std::string output = str.toStdString();

  // -- to not be on the null character
  auto back_iterator = --(output.cend());
  std::string cor_number;
  for (; back_iterator != output.begin(); --back_iterator) {
    if (*back_iterator == '\n') {
      // found the last new line
      cor_number = std::string(back_iterator + 1, output.cend());
      break;
    }
  }
  int cor = 0;
  try {
    cor = std::stoi(cor_number);
  } catch (std::invalid_argument &) {
    // output COR cannot be converted, do not change anything and just return
    // silently
    return;
  }
  m_ui.spinBox_cor_x->setValue(cor);
  // middle of image is ((bottom - top) / 2)
  // center is (top location + middle)
  int imageCentre =
      m_ui.spinBox_roi_top->value() +
      ((m_ui.spinBox_roi_bottom->value() - m_ui.spinBox_roi_top->value()) / 2);

  m_ui.spinBox_cor_y->setValue(imageCentre);

  // refresh all coordinate variables
  grabCoRFromWidgets();
  grabROIFromWidgets();
  grabNormAreaFromWidgets();
  // redraw the image with the rectangles
  refreshImage();
}

} // namespace CustomInterfaces
} // namespace MantidQt
