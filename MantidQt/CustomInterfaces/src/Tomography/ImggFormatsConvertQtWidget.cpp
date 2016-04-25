#include "MantidQtCustomInterfaces/Tomography/ImggFormatsConvertQtWidget.h"
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

#include <iostream>
#include <QImage>

namespace MantidQt {
namespace CustomInterfaces {

// this would be more like a CustomWidget if it's eventually moved there
const std::string ImggFormatsConvertQtWidget::m_settingsGroup =
    "CustomInterfaces/ImggFormatsConvertView";

ImggFormatsConvertQtWidget::ImggFormatsConvertQtWidget(QWidget *parent)
    : QWidget(parent), IImggFormatsConvertView(), m_presenter(nullptr) {
  initLayout();
}

void ImggFormatsConvertQtWidget::userWarning(const std::string &warn,
                                             const std::string &description) {
  QMessageBox::warning(this, QString::fromStdString(warn),
                       QString::fromStdString(description), QMessageBox::Ok,
                       QMessageBox::Ok);
}

void ImggFormatsConvertQtWidget::userError(const std::string &err,
                                           const std::string &description) {
  QMessageBox::critical(this, QString::fromStdString(err),
                        QString::fromStdString(description), QMessageBox::Ok,
                        QMessageBox::Ok);
}

void ImggFormatsConvertQtWidget::setFormats(
    const std::vector<std::string> &fmts, const std::vector<bool> &enable) {
  // same formats for inputs and outputs
  setFormatsCombo(m_ui.comboBox_input_format, fmts, enable);
  setFormatsCombo(m_ui.comboBox_output_format, fmts, enable);

  m_ui.spinBox_max_search_depth->setValue(3);
  if (m_ui.comboBox_output_format->count() > 0) {
    m_ui.comboBox_output_format->setCurrentIndex(1);
  }
}

void ImggFormatsConvertQtWidget::setFormatsCombo(
    QComboBox *cbox, const std::vector<std::string> &fmts,
    const std::vector<bool> &enable) {
  cbox->clear();
  for (const auto &name : fmts) {
    cbox->addItem(QString::fromStdString(name));
  }

  if (enable.empty() || enable.size() != fmts.size())
    return;

  // disable
}

std::string ImggFormatsConvertQtWidget::inputPath() const {
  return m_ui.lineEdit_input_path->text().toStdString();
}

std::string ImggFormatsConvertQtWidget::inputFormatName() const {
  const auto cbox = m_ui.comboBox_input_format;
  if (!cbox)
    return "";

  return cbox->currentText().toStdString();
}

std::string ImggFormatsConvertQtWidget::outputPath() const {
  return m_ui.lineEdit_output_path->text().toStdString();
}

std::string ImggFormatsConvertQtWidget::outputFormatName() const {
  const auto cbox = m_ui.comboBox_output_format;
  if (!cbox)
    return "";

  return cbox->currentText().toStdString();
}

bool ImggFormatsConvertQtWidget::compressHint() const {
  return 0 == m_ui.comboBox_compression->currentIndex();
}

void ImggFormatsConvertQtWidget::convert(
    const std::string &inputName, const std::string &inputFormat,
    const std::string &outputName, const std::string &outputFormat) const {

  QImage img = loadImgFile(inputName, inputFormat);

  if (!img.isGrayscale()) {
    // Qt5 has QImage::Format_Alpha8;
    QImage::Format toFormat = QImage::Format_RGB32;
    Qt::ImageConversionFlag toFlags = Qt::MonoOnly;
    img.convertToFormat(toFormat, toFlags);
  }

  writeImgFile(img, outputName, outputFormat);
}

void ImggFormatsConvertQtWidget::writeImg(MatrixWorkspace_sptr inWks,
                                          const std::string &outputName,
                                          const std::string &outFormat) const {

  size_t width = inWks->getNumberHistograms();
  if (0 == width)
    return;
  size_t height = inWks->blocksize();
  QImage img(QSize(static_cast<int>(width), static_cast<int>(height)),
             QImage::Format_Indexed8);

  int tableSize = 256;
  QVector<QRgb> grayscale;
  grayscale.resize(tableSize);
  for (int i = 0; i < grayscale.size(); i++) {
    grayscale.push_back(qRgb(i, i, i));
  }
  const double scaleFactor = std::numeric_limits<unsigned char>::max();
  for (int yi = 0; yi < static_cast<int>(width); ++yi) {
    const auto &row = inWks->readY(yi);
    for (int xi = 0; xi < static_cast<int>(width); ++xi) {
      const int scaled = static_cast<int>(row[xi] / scaleFactor);
      QRgb vRgb = qRgb(scaled, scaled, scaled);
      img.setPixel(xi, yi, vRgb);
    }
  }

  writeImgFile(img, outputName, outFormat);
}

/**
 * Write an image using a QImageWriter
 *
 * @param QImage with data ready
 * @param outputName output filename
 * @param outFormat format for the image file
 */
void ImggFormatsConvertQtWidget::writeImgFile(
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
ImggFormatsConvertQtWidget::loadImg(const std::string &inputName,
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
    const auto &row = imgWks->getSpectrum(yi);
    auto &dataY = row->dataY();
    auto &dataX = row->dataX();
    std::fill(dataX.begin(), dataX.end(), static_cast<double>(yi));
    for (int xi = 0; xi < static_cast<int>(width); ++xi) {
      QRgb vRgb = img.pixel(xi, yi);
      dataY[xi] = scaleFactor * qGray(vRgb);
    }
  }

  return imgWks;
}

/**
 * Write an image using a QImageReader
 *
 * @param outputName output filename
 * @param outFormat format for the image file
 *
 * @return QImage object with image data read from file
 */
QImage
ImggFormatsConvertQtWidget::loadImgFile(const std::string &inputName,
                                        const std::string inFormat) const {
  // Simpler but less flexible load with QImage:
  // img.load(QString::fromStdString(inputName));

  QImageReader reader(inputName.c_str());
  if (!reader.autoDetectImageFormat()) {
    reader.setFormat(inFormat.c_str());
  }

  return reader.read();
}

size_t ImggFormatsConvertQtWidget::maxSearchDepth() const {
  return static_cast<size_t>(m_ui.spinBox_max_search_depth->value());
}

void ImggFormatsConvertQtWidget::initLayout() {
  // setup container ui
  m_ui.setupUi(this);

  readSettings();

  setup();
  // presenter that knows how to handle a view like this. It should
  // take care of all the logic. Note the view needs to now the
  // concrete presenter here
  m_presenter.reset(new ImggFormatsConvertPresenter(this));

  // it will know what compute resources and tools we have available:
  // This view doesn't even know the names of compute resources, etc.
  m_presenter->notify(IImggFormatsConvertPresenter::Init);
}

void ImggFormatsConvertQtWidget::setup() {

  connect(m_ui.pushButton_browse_input, SIGNAL(released()), this,
          SLOT(browseImgInputConvertClicked()));

  connect(m_ui.pushButton_browse_output, SIGNAL(released()), this,
          SLOT(browseImgOutputConvertClicked()));

  connect(m_ui.pushButton_convert, SIGNAL(released()), this,
          SLOT(convertClicked()));
}

void ImggFormatsConvertQtWidget::readSettings() {
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

void ImggFormatsConvertQtWidget::saveSettings() const {
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

void ImggFormatsConvertQtWidget::browseImgInputConvertClicked() {
  grabUserBrowseDir(m_ui.lineEdit_input_path);
}

void ImggFormatsConvertQtWidget::browseImgOutputConvertClicked() {
  grabUserBrowseDir(m_ui.lineEdit_output_path);
}

void ImggFormatsConvertQtWidget::convertClicked() {
  m_presenter->notify(IImggFormatsConvertPresenter::Convert);
}

std::string ImggFormatsConvertQtWidget::grabUserBrowseDir(
    QLineEdit *le, const std::string &userMsg, bool remember) {

  QString prev;
  if (le->text().isEmpty()) {
    prev =
        MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  } else {
    prev = le->text();
  }

  QString path(QFileDialog::getExistingDirectory(
      this, tr(QString::fromStdString(userMsg)), prev));

  if (!path.isEmpty()) {
    le->setText(path);
    if (remember) {
      MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(
          path);
    }
  }

  return path.toStdString();
}

std::string ImggFormatsConvertQtWidget::askImgOrStackPath() {
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

void ImggFormatsConvertQtWidget::closeEvent(QCloseEvent *event) {
  m_presenter->notify(IImggFormatsConvertPresenter::ShutDown);
  event->accept();
}

} // namespace CustomInterfaces
} // namespace MantidQt
