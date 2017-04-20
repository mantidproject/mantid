#include "MantidQtCustomInterfaces/Tomography/ImggFormatsConvertViewQtWidget.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtCustomInterfaces/Tomography/ImggFormatsConvertPresenter.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

#include <QCloseEvent>
#include <QFileDialog>
#include <QImageReader>
#include <QImageWriter>
#include <QMessageBox>
#include <QSettings>

#include <QImage>
#include <iostream>

namespace MantidQt {
namespace CustomInterfaces {

// this would be more like a CustomWidget if it's eventually moved there
const std::string ImggFormatsConvertViewQtWidget::m_settingsGroup =
    "CustomInterfaces/ImggFormatsConvertView";

ImggFormatsConvertViewQtWidget::ImggFormatsConvertViewQtWidget(QWidget *parent)
    : QWidget(parent), IImggFormatsConvertView(), m_presenter(nullptr) {
  initLayout();
}

ImggFormatsConvertViewQtWidget::~ImggFormatsConvertViewQtWidget() {
  m_presenter->notify(IImggFormatsConvertPresenter::ShutDown);
}

void ImggFormatsConvertViewQtWidget::initLayout() {
  // setup container ui
  m_ui.setupUi(this);

  setup();
  // presenter that knows how to handle a view like this. It should
  // take care of all the logic. Note the view needs to now the
  // concrete presenter here
  m_presenter.reset(new ImggFormatsConvertPresenter(this));

  // it will know what compute resources and tools we have available:
  // This view doesn't even know the names of compute resources, etc.
  m_presenter->notify(IImggFormatsConvertPresenter::Init);

  // some combo boxes are re-populated by the init process, reload
  // settings now:
  readSettings();
}

void ImggFormatsConvertViewQtWidget::setup() {

  connect(m_ui.pushButton_browse_input, SIGNAL(released()), this,
          SLOT(browseImgInputConvertClicked()));

  connect(m_ui.pushButton_browse_output, SIGNAL(released()), this,
          SLOT(browseImgOutputConvertClicked()));

  connect(m_ui.pushButton_convert, SIGNAL(released()), this,
          SLOT(convertClicked()));
}

void ImggFormatsConvertViewQtWidget::readSettings() {
  QSettings qs;
  qs.beginGroup(QString::fromStdString(m_settingsGroup));

  m_ui.comboBox_input_format->setCurrentIndex(
      qs.value("input-format", 0).toInt());
  m_ui.lineEdit_input_path->setText(qs.value("input-path", "").toString());

  m_ui.comboBox_output_format->setCurrentIndex(
      qs.value("output-format", 0).toInt());
  m_ui.comboBox_bit_depth->setCurrentIndex(qs.value("bit-depth", 0).toInt());
  m_ui.comboBox_compression->setCurrentIndex(
      qs.value("compression", 0).toInt());
  m_ui.spinBox_max_search_depth->setValue(
      qs.value("max-search-depth", 0).toInt());
  m_ui.lineEdit_output_path->setText(qs.value("output-path", "").toString());

  restoreGeometry(qs.value("interface-win-geometry").toByteArray());
  qs.endGroup();
}

void ImggFormatsConvertViewQtWidget::saveSettings() const {
  QSettings qs;
  qs.beginGroup(QString::fromStdString(m_settingsGroup));

  qs.setValue("input-format", m_ui.comboBox_input_format->currentIndex());
  qs.setValue("input-path", m_ui.lineEdit_input_path->text());

  qs.setValue("output-format", m_ui.comboBox_output_format->currentIndex());
  qs.setValue("bit-depth", m_ui.comboBox_bit_depth->currentIndex());
  qs.setValue("compression", m_ui.comboBox_compression->currentIndex());
  qs.setValue("max-search-depth", m_ui.spinBox_max_search_depth->value());
  qs.setValue("output-path", m_ui.lineEdit_output_path->text());

  qs.setValue("interface-win-geometry", saveGeometry());
  qs.endGroup();
}

void ImggFormatsConvertViewQtWidget::browseImgInputConvertClicked() {
  grabUserBrowseDir(m_ui.lineEdit_input_path);
}

void ImggFormatsConvertViewQtWidget::browseImgOutputConvertClicked() {
  grabUserBrowseDir(m_ui.lineEdit_output_path);
}

void ImggFormatsConvertViewQtWidget::convertClicked() {
  m_presenter->notify(IImggFormatsConvertPresenter::Convert);
}

void ImggFormatsConvertViewQtWidget::userWarning(
    const std::string &warn, const std::string &description) {
  QMessageBox::warning(this, QString::fromStdString(warn),
                       QString::fromStdString(description), QMessageBox::Ok,
                       QMessageBox::Ok);
}

void ImggFormatsConvertViewQtWidget::userError(const std::string &err,
                                               const std::string &description) {
  QMessageBox::critical(this, QString::fromStdString(err),
                        QString::fromStdString(description), QMessageBox::Ok,
                        QMessageBox::Ok);
}

void ImggFormatsConvertViewQtWidget::setFormats(
    const std::vector<std::string> &fmts, const std::vector<bool> &enableLoad,
    const std::vector<bool> &enableSave) {
  // same formats for inputs and outputs
  setFormatsCombo(m_ui.comboBox_input_format, fmts, enableLoad);
  setFormatsCombo(m_ui.comboBox_output_format, fmts, enableSave);

  m_ui.spinBox_max_search_depth->setValue(3);
  if (m_ui.comboBox_output_format->count() > 0) {
    m_ui.comboBox_output_format->setCurrentIndex(1);
  }
}

void ImggFormatsConvertViewQtWidget::setFormatsCombo(
    QComboBox *cbox, const std::vector<std::string> &fmts,
    const std::vector<bool> &enable) {
  cbox->clear();
  for (const auto &name : fmts) {
    cbox->addItem(QString::fromStdString(name));
  }

  if (enable.empty() || enable.size() != fmts.size())
    return;

  for (size_t fmtIdx = 0; fmtIdx < fmts.size(); fmtIdx++) {
    if (!enable[fmtIdx]) {
      // to display the text in this row as "disabled"
      QModelIndex rowIdx = cbox->model()->index(static_cast<int>(fmtIdx), 0);
      QVariant disabled(0);
      cbox->model()->setData(rowIdx, disabled, Qt::UserRole - 1);
    }
  }
}

std::string ImggFormatsConvertViewQtWidget::inputPath() const {
  return m_ui.lineEdit_input_path->text().toStdString();
}

std::string ImggFormatsConvertViewQtWidget::inputFormatName() const {
  const auto cbox = m_ui.comboBox_input_format;
  if (!cbox)
    return "";

  return cbox->currentText().toStdString();
}

std::string ImggFormatsConvertViewQtWidget::outputPath() const {
  return m_ui.lineEdit_output_path->text().toStdString();
}

std::string ImggFormatsConvertViewQtWidget::outputFormatName() const {
  const auto cbox = m_ui.comboBox_output_format;
  if (!cbox)
    return "";

  return cbox->currentText().toStdString();
}

bool ImggFormatsConvertViewQtWidget::compressHint() const {
  return 0 == m_ui.comboBox_compression->currentIndex();
}

void ImggFormatsConvertViewQtWidget::convert(
    const std::string &inputName, const std::string &inputFormat,
    const std::string &outputName, const std::string &outputFormat) const {

  QImage img = loadImgFile(inputName, inputFormat);

  if (!img.isGrayscale()) {
    // Qt5 has QImage::Format_Alpha8;
    QImage::Format toFormat = QImage::Format_RGB32;
    Qt::ImageConversionFlag toFlags = Qt::MonoOnly;
    img = img.convertToFormat(toFormat, toFlags);
  }

  writeImgFile(img, outputName, outputFormat);
}

void ImggFormatsConvertViewQtWidget::writeImg(
    MatrixWorkspace_sptr inWks, const std::string &outputName,
    const std::string &outFormat) const {
  if (!inWks)
    return;
  auto width = inWks->getNumberHistograms();
  if (0 == width)
    return;
  auto height = inWks->blocksize();

  QImage img(QSize(static_cast<int>(width), static_cast<int>(height)),
             QImage::Format_Indexed8);

  int tableSize = 256;
  QVector<QRgb> grayscale(tableSize);
  for (int i = 0; i < grayscale.size(); i++) {
    int level = i; // would be equivalent: qGray(i, i, i);
    grayscale[i] = qRgb(level, level, level);
  }
  img.setColorTable(grayscale);

  // only 16 to 8 bits color map supported with current libraries
  const double scaleFactor = std::numeric_limits<unsigned short int>::max() /
                             std::numeric_limits<unsigned char>::max();
  for (int yi = 0; yi < static_cast<int>(width); ++yi) {
    const auto &row = inWks->y(yi);
    for (int xi = 0; xi < static_cast<int>(width); ++xi) {
      int scaled = static_cast<int>(row[xi] / scaleFactor);
      // Images not from IMAT, just crop. This needs much improvement when
      // we have proper Load/SaveImage algorithms
      if (scaled > 255)
        scaled = 255;
      if (scaled < 0)
        scaled = 0;
      img.setPixel(xi, yi, scaled);
    }
  }

  writeImgFile(img, outputName, outFormat);
}

/**
 * Write an image using a QImageWriter
 *
 * @param img QImage with data ready to be saved
 * @param outputName output filename
 * @param outFormat format for the image file
 */
void ImggFormatsConvertViewQtWidget::writeImgFile(
    const QImage &img, const std::string &outputName,
    const std::string &outFormat) const {
  // With (simpler but less flexible) QImage:
  // img.save(QString::fromStdString(outputName));

  QImageWriter writer(QString::fromStdString(outputName));
  writer.setFormat(outFormat.c_str());
  if (compressHint())
    writer.setCompression(1);
  writer.write(img);
}

MatrixWorkspace_sptr
ImggFormatsConvertViewQtWidget::loadImg(const std::string &inputName,
                                        const std::string &inFormat) const {

  QImage img = loadImgFile(inputName, inFormat);
  int width = img.width();
  int height = img.height();

  MatrixWorkspace_sptr imgWks = boost::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create("Workspace2D", height, width + 1,
                                          width));
  imgWks->setTitle(inputName);
  const double scaleFactor = std::numeric_limits<unsigned char>::max();
  for (int yi = 0; yi < static_cast<int>(width); ++yi) {
    auto &row = imgWks->getSpectrum(yi);
    auto &dataY = row.mutableY();
    row.mutableX() = static_cast<double>(yi);
    for (int xi = 0; xi < static_cast<int>(width); ++xi) {
      QRgb vRgb = img.pixel(xi, yi);
      dataY[xi] = scaleFactor * qGray(vRgb);
    }
  }

  return imgWks;
}

/**
 * Load an image using a QImageReader
 *
 * @param inputName filename
 * @param inFormat format for the image file
 *
 * @return QImage object with image data read from file
 */
QImage
ImggFormatsConvertViewQtWidget::loadImgFile(const std::string &inputName,
                                            const std::string inFormat) const {
  // Simpler but less flexible load with QImage:
  // img.load(QString::fromStdString(inputName));

  QImageReader reader(inputName.c_str());
  if (!reader.autoDetectImageFormat()) {
    reader.setFormat(inFormat.c_str());
  }

  return reader.read();
}

size_t ImggFormatsConvertViewQtWidget::maxSearchDepth() const {
  return static_cast<size_t>(m_ui.spinBox_max_search_depth->value());
}

std::string ImggFormatsConvertViewQtWidget::grabUserBrowseDir(
    QLineEdit *le, const std::string &userMsg, bool remember) {

  QString prev;
  if (le->text().isEmpty()) {
    prev =
        MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  } else {
    prev = le->text();
  }

  QString path(
      QFileDialog::getExistingDirectory(this, tr(userMsg.c_str()), prev));

  if (!path.isEmpty()) {
    le->setText(path);
    if (remember) {
      MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(
          path);
    }
  }

  return path.toStdString();
}

std::string ImggFormatsConvertViewQtWidget::askImgOrStackPath() {
  // get path
  QString fitsStr = QString("Supported formats: FITS, TIFF and PNG "
                            "(*.fits *.fit *.tiff *.tif *.png);;"
                            "FITS, Flexible Image Transport System images "
                            "(*.fits *.fit);;"
                            "TIFF, Tagged Image File Format "
                            "(*.tif *.tiff);;"
                            "PNG, Portable Network Graphics "
                            "(*.png);;"
                            "Other extensions/all files (*)");
  QString prevPath =
      MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  QString path(QFileDialog::getExistingDirectory(
      this, tr("Open stack of images"), prevPath, QFileDialog::ShowDirsOnly));
  if (!path.isEmpty()) {
    MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(path);
  }

  return path.toStdString();
}

void ImggFormatsConvertViewQtWidget::closeEvent(QCloseEvent *event) {
  m_presenter->notify(IImggFormatsConvertPresenter::ShutDown);
  event->accept();
}

} // namespace CustomInterfaces
} // namespace MantidQt
