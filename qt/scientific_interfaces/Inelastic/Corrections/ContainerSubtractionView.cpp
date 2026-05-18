// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ContainerSubtractionView.h"

#include "ContainerSubtractionPresenter.h"

using namespace Mantid::API;

namespace MantidQt::CustomInterfaces {
static const std::map<CSCurves, std::pair<QString, QColor>> CSPlotCurves = {
    {CSCurves::SAMPLE, {"Sample", Qt::black}},
    {CSCurves::CONTAINER, {"Container", Qt::red}},
    {CSCurves::SUBTRACTED, {"Subtracted", Qt::blue}}};

ContainerSubtractionView::ContainerSubtractionView(QWidget *parent) : QWidget(parent), m_presenter() {
  m_uiForm.setupUi(parent);

  connect(m_uiForm.dsSample, &DataSelector::dataReady, this, &ContainerSubtractionView::notifySampleDataReady);
  connect(m_uiForm.dsContainer, &DataSelector::dataReady, this, &ContainerSubtractionView::notifyCanDataReady);
  connect(m_uiForm.spPreviewSpec, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
          &ContainerSubtractionView::notifySpectraIncreaseClicked);
  connect(m_uiForm.ckShiftCan, &QCheckBox::toggled, m_uiForm.spShift, &QDoubleSpinBox::setEnabled);
  connect(m_uiForm.ckScaleCan, &QCheckBox::toggled, m_uiForm.spCanScale, &QDoubleSpinBox::setEnabled);
  connect(m_uiForm.spCanScale, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this,
          &ContainerSubtractionView::notifyUpdateCan);
  connect(m_uiForm.spShift, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this,
          &ContainerSubtractionView::notifyUpdateCan);
  connect(m_uiForm.pbSave, &QPushButton::clicked, this, &ContainerSubtractionView::notifySaveClicked);
  connect(m_uiForm.pbPlotPreview, &QPushButton::clicked, this, &ContainerSubtractionView::notifyPreviewClicked);

  // Allows empty workspace selector when initially selected
  m_uiForm.dsSample->isOptional(true);
  m_uiForm.dsContainer->isOptional(true);

  m_uiForm.spPreviewSpec->setMinimum(0);
  m_uiForm.spPreviewSpec->setMaximum(0);
}

void ContainerSubtractionView::notifySampleDataReady(const QString &dataName) const {
  m_presenter->handleSampleReady(dataName.toStdString());
}

void ContainerSubtractionView::notifyCanDataReady(const QString &dataName) const {
  QSignalBlocker blockSpShift(m_uiForm.spShift);
  QSignalBlocker blockerSpCan(m_uiForm.spCanScale);
  m_uiForm.spShift->setValue(0.0);
  m_uiForm.spCanScale->setValue(1.0);
  m_presenter->handleCanReady(dataName.toStdString());
}

void ContainerSubtractionView::subscribe(IContainerSubtractionPresenter *presenter) { m_presenter = presenter; }

void ContainerSubtractionView::notifySaveClicked() const { m_presenter->handleSaveClicked(); }
void ContainerSubtractionView::notifyPreviewClicked() const { m_presenter->handlePlotPreviewClicked(); }
void ContainerSubtractionView::notifySpectraIncreaseClicked(int specNo) const { m_presenter->updatePlot(specNo); }
void ContainerSubtractionView::notifyUpdateCan() const { m_presenter->handleUpdateContainerPlot(); }

IOutputPlotOptionsView *ContainerSubtractionView::getPlotOptions() const { return m_uiForm.ipoPlotOptions; }
IRunView *ContainerSubtractionView::getRunView() const { return m_uiForm.runWidget; }
IOutputNameView *ContainerSubtractionView::getOutputNameView() const { return m_uiForm.outputNameWidget; }

void ContainerSubtractionView::plotSpectrum(const CSCurves &curveName, const MatrixWorkspace_sptr &ws, size_t specNo) {
  const auto &[qCurveName, qCurveColor] = CSPlotCurves.at(curveName);
  m_uiForm.ppPreview->addSpectrum(qCurveName, ws, specNo, qCurveColor);
  m_uiForm.spPreviewSpec->setValue(static_cast<int>(specNo));
}

void ContainerSubtractionView::validate(IUserInputValidator *validator) {
  validator->checkDataSelectorIsValid("Sample", m_uiForm.dsSample);
  validator->checkDataSelectorIsValid("Container", m_uiForm.dsContainer);
}

void ContainerSubtractionView::loadSettings(const QSettings &settings) {
  m_uiForm.dsContainer->readSettings(settings.group());
  m_uiForm.dsSample->readSettings(settings.group());
}

void ContainerSubtractionView::setSampleFBSuffixes(const QStringList &suffixes) {
  m_uiForm.dsSample->setFBSuffixes(suffixes);
}
void ContainerSubtractionView::setSampleWSSuffixes(const QStringList &suffixes) {
  m_uiForm.dsSample->setWSSuffixes(suffixes);
}
void ContainerSubtractionView::setCanFBSuffixes(const QStringList &suffixes) {
  m_uiForm.dsContainer->setFBSuffixes(suffixes);
}
void ContainerSubtractionView::setCanWSSuffixes(const QStringList &suffixes) {
  m_uiForm.dsContainer->setWSSuffixes(suffixes);
}

void ContainerSubtractionView::clearPlot() const { m_uiForm.ppPreview->clear(); }

double ContainerSubtractionView::getShift() const {
  return m_uiForm.ckShiftCan->isChecked() ? m_uiForm.spShift->value() : 0.0;
}
double ContainerSubtractionView::getScale() const {
  return m_uiForm.ckScaleCan->isChecked() ? m_uiForm.spCanScale->value() : 1.0;
}
int ContainerSubtractionView::getSpNo() const { return m_uiForm.spPreviewSpec->value(); }

int ContainerSubtractionView::getSpMax() const { return m_uiForm.spPreviewSpec->maximum(); }

void ContainerSubtractionView::setSpMax(int max) { m_uiForm.spPreviewSpec->setMaximum(max); }

void ContainerSubtractionView::enableSaveButton(bool enable) { m_uiForm.pbSave->setEnabled(enable); }

void ContainerSubtractionView::setLoadHistory(bool doLoadHistory) {
  m_uiForm.dsSample->setLoadProperty("LoadHistory", doLoadHistory);
  m_uiForm.dsContainer->setLoadProperty("LoadHistory", doLoadHistory);
}

bool ContainerSubtractionView::requestRebinToSample() {
  const char *text = "Binning on sample and container does not match."
                     "Would you like to rebin the container to match the sample?";

  const int result = QMessageBox::question(this, tr("Rebin container?"), tr(text), QMessageBox::Yes, QMessageBox::No,
                                           QMessageBox::NoButton);

  return result == QMessageBox::Yes;
}

} // namespace MantidQt::CustomInterfaces
