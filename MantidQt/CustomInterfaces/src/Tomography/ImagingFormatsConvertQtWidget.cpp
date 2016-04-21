#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtCustomInterfaces/Tomography/ImagingFormatsConvertPresenter.h"
#include "MantidQtCustomInterfaces/Tomography/ImagingFormatsConvertQtWidget.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

namespace MantidQt {
namespace CustomInterfaces {

// this would be more like a CustomWidget if it's eventually moved there
const std::string ImagingFormatsConvertQtWidget::m_settingsGroup =
    "CustomInterfaces/ImagingFormatsConvertView";

ImagingFormatsConvertQtWidget::ImagingFormatsConvertQtWidget(QWidget *parent)
    : QWidget(parent), IImagingFormatsConvertView(), m_presenter(nullptr) {
  initLayout();
}

void ImagingFormatsConvertQtWidget::userWarning(
    const std::string &warn, const std::string &description) {
  QMessageBox::warning(this, QString::fromStdString(warn),
                       QString::fromStdString(description), QMessageBox::Ok,
                       QMessageBox::Ok);
}

void ImagingFormatsConvertQtWidget::userError(const std::string &err,
                                              const std::string &description) {
  QMessageBox::critical(this, QString::fromStdString(err),
                        QString::fromStdString(description), QMessageBox::Ok,
                        QMessageBox::Ok);
}

void ImagingFormatsConvertQtWidget::setFormats(
    const std::vector<std::string> &fmts, const std::vector<bool> &enable) {
  // same formats for inputs and outputs
  setFormatsCombo(m_ui.comboBox_input_format, fmts, enable);
  setFormatsCombo(m_ui.comboBox_output_format, fmts, enable);

  if (m_ui.comboBox_output_format->count() > 0) {
    m_ui.comboBox_output_format->setCurrentIndex(1);
  }
}

void ImagingFormatsConvertQtWidget::setFormatsCombo(
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

std::string ImagingFormatsConvertQtWidget::inputPath() const {
  return m_ui.lineEdit_input->text().toStdString();
}

std::string ImagingFormatsConvertQtWidget::inputFormatName() const {
  const auto cbox = m_ui.comboBox_input_format;
  if (!cbox)
    return "";

  return cbox->itemData(cbox->currentIndex()).toString().toStdString();
}

std::string ImagingFormatsConvertQtWidget::outputPath() const {
  return m_ui.lineEdit_output->text().toStdString();
}

std::string ImagingFormatsConvertQtWidget::outputFormatName() const {
  const auto cbox = m_ui.comboBox_output_format;
  if (!cbox)
    return "";
  return cbox->itemData(cbox->currentIndex()).toString().toStdString();
}

bool ImagingFormatsConvertQtWidget::compressHint() const {
  return 0 == m_ui.comboBox_compression->currentIndex();
}

void ImagingFormatsConvertQtWidget::saveSettings() const {
  QSettings qs;
  qs.beginGroup(QString::fromStdString(m_settingsGroup));

  qs.setValue("interface-win-geometry", saveGeometry());
  qs.endGroup();
}

void ImagingFormatsConvertQtWidget::initLayout() {
  // setup container ui
  m_ui.setupUi(this);

  setup();
  // presenter that knows how to handle a IImageROIView should take care
  // of all the logic. Note the view needs to now the concrete presenter
  // here
  m_presenter.reset(new ImagingFormatsConvertPresenter(this));

  // it will know what compute resources and tools we have available:
  // This view doesn't even know the names of compute resources, etc.
  m_presenter->notify(IImagingFormatsConvertPresenter::Init);
}

void ImagingFormatsConvertQtWidget::setup() {
  connect(m_ui.pushButton_browse_input, SIGNAL(released()), this,
          SLOT(browseImgInputConvertClicked()));

  connect(m_ui.pushButton_browse_output, SIGNAL(released()), this,
          SLOT(browseImgOutputConvertClicked()));

  connect(m_ui.pushButton_browse_output, SIGNAL(released()), this,
          SLOT(convertClicked()));
}

void ImagingFormatsConvertQtWidget::browseImgInputConvertClicked() {
  grabUserBrowseDir(m_ui.lineEdit_input);
}

void ImagingFormatsConvertQtWidget::browseImgOutputConvertClicked() {
  grabUserBrowseDir(m_ui.lineEdit_output);
}

void ImagingFormatsConvertQtWidget::convertClicked() {
  m_presenter->notify(IImagingFormatsConvertPresenter::Convert);
}

std::string ImagingFormatsConvertQtWidget::grabUserBrowseDir(
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

std::string ImagingFormatsConvertQtWidget::askImgOrStackPath() {
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

void ImagingFormatsConvertQtWidget::closeEvent(QCloseEvent *event) {
  m_presenter->notify(IImagingFormatsConvertPresenter::ShutDown);
  event->accept();
}

} // namespace CustomInterfaces
} // namespace MantidQt
