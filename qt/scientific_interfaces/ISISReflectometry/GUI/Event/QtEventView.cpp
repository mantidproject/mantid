// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtEventView.h"
#include "EventPresenter.h"
#include "MantidKernel/UsageService.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** Constructor
 * @param parent :: [input] The parent of this widget
 */
QtEventView::QtEventView(QWidget *parent) : QWidget(parent) { initLayout(); }

void QtEventView::subscribe(EventViewSubscriber *notifyee) {
  m_notifyee = notifyee;
}

void QtEventView::initLayout() {
  m_ui.setupUi(this);
  initUniformSliceTypeLayout();
  initUniformEvenSliceTypeLayout();
  initLogValueSliceTypeLayout();
  initCustomSliceTypeLayout();
  connect(m_ui.disabledSlicingButton, SIGNAL(toggled(bool)), this,
          SLOT(onToggleDisabledSlicing(bool)));

  m_sliceTypeRadioButtons = makeQWidgetGroup(
      m_ui.uniformEvenButton, m_ui.uniformButton, m_ui.logValueButton,
      m_ui.customButton, m_ui.disabledSlicingButton);
}

void QtEventView::initUniformSliceTypeLayout() {
  m_uniformGroup = makeQWidgetGroup(m_ui.uniformEdit, m_ui.uniformLabel);
  connect(m_ui.uniformButton, SIGNAL(toggled(bool)), this,
          SLOT(onToggleUniform(bool)));

  connect(m_ui.uniformEvenEdit, SIGNAL(valueChanged(int)), this,
          SLOT(onUniformEvenChanged(int)));
  connect(m_ui.uniformEdit, SIGNAL(valueChanged(double)), this,
          SLOT(onUniformSecondsChanged(double)));

  connect(m_ui.customEdit, SIGNAL(textChanged(QString const &)), this,
          SLOT(onCustomChanged(QString const &)));
  connect(m_ui.logValueEdit, SIGNAL(textChanged(QString const &)), this,
          SLOT(onLogValuesChanged(QString const &)));
  connect(m_ui.logValueTypeEdit, SIGNAL(textChanged(QString const &)), this,
          SLOT(onLogValueTypeChanged(QString const &)));
}

void QtEventView::onUniformEvenChanged(int numberOfSlices) {
  m_notifyee->notifyUniformSliceCountChanged(numberOfSlices);
}

void QtEventView::onUniformSecondsChanged(double numberOfSeconds) {
  m_notifyee->notifyUniformSecondsChanged(numberOfSeconds);
}

void QtEventView::onCustomChanged(QString const &listOfSlices) {
  m_notifyee->notifyCustomSliceValuesChanged(listOfSlices.toStdString());
}

void QtEventView::onLogValuesChanged(QString const &listOfSliceBreakpoints) {
  m_notifyee->notifyLogSliceBreakpointsChanged(
      listOfSliceBreakpoints.toStdString());
}

void QtEventView::onLogValueTypeChanged(QString const &logBlockName) {
  m_notifyee->notifyLogBlockNameChanged(logBlockName.toStdString());
}

void QtEventView::initUniformEvenSliceTypeLayout() {
  m_uniformEvenGroup =
      makeQWidgetGroup(m_ui.uniformEvenEdit, m_ui.uniformEvenLabel);
  connect(m_ui.uniformEvenButton, SIGNAL(toggled(bool)), this,
          SLOT(onToggleUniformEven(bool)));
}

void QtEventView::initCustomSliceTypeLayout() {
  m_customGroup = makeQWidgetGroup(m_ui.customEdit, m_ui.customLabel);
  connect(m_ui.customButton, SIGNAL(toggled(bool)), this,
          SLOT(onToggleCustom(bool)));
}

void QtEventView::initLogValueSliceTypeLayout() {
  m_logValueGroup =
      makeQWidgetGroup(m_ui.logValueTypeEdit, m_ui.logValueTypeLabel,
                       m_ui.logValueEdit, m_ui.logValueLabel);
  connect(m_ui.logValueButton, SIGNAL(toggled(bool)), this,
          SLOT(onToggleLogValue(bool)));
}

void QtEventView::enableSliceType(SliceType sliceType) {
  switch (sliceType) {
  case SliceType::Uniform:
    m_uniformGroup.enable();
    break;
  case SliceType::UniformEven:
    m_uniformEvenGroup.enable();
    break;
  case SliceType::Custom:
    m_customGroup.enable();
    break;
  case SliceType::LogValue:
    m_logValueGroup.enable();
    break;
  case SliceType::None:
    break;
  }
}

void QtEventView::disableSliceType(SliceType sliceType) {
  switch (sliceType) {
  case SliceType::Uniform:
    m_uniformGroup.disable();
    break;
  case SliceType::UniformEven:
    m_uniformEvenGroup.disable();
    break;
  case SliceType::Custom:
    m_customGroup.disable();
    break;
  case SliceType::LogValue:
    m_logValueGroup.disable();
    break;
  case SliceType::None:
    break;
  }
}

std::string QtEventView::logBlockName() const {
  return textFrom(m_ui.logValueTypeEdit);
}

std::string QtEventView::logBreakpoints() const {
  return textFrom(m_ui.logValueEdit);
}

std::string QtEventView::customBreakpoints() const {
  return textFrom(m_ui.customEdit);
}

void showAsInvalid(QLineEdit &lineEdit) {
  auto palette = lineEdit.palette();
  palette.setColor(QPalette::Base, QColor("#ffb8ad"));
  lineEdit.setPalette(palette);
}

void showAsValid(QLineEdit &lineEdit) {
  auto palette = lineEdit.palette();
  palette.setColor(QPalette::Base, Qt::transparent);
  lineEdit.setPalette(palette);
}

void QtEventView::showCustomBreakpointsInvalid() {
  showAsInvalid(*m_ui.customEdit);
}

void QtEventView::showCustomBreakpointsValid() {
  showAsValid(*m_ui.customEdit);
}

void QtEventView::showLogBreakpointsInvalid() {
  showAsInvalid(*m_ui.logValueEdit);
}

void QtEventView::showLogBreakpointsValid() { showAsValid(*m_ui.logValueEdit); }

int QtEventView::uniformSliceCount() const {
  return m_ui.uniformEvenEdit->value();
}

double QtEventView::uniformSliceLength() const {
  return m_ui.uniformEdit->value();
}

std::string QtEventView::textFrom(QLineEdit const *const widget) const {
  return widget->text().toStdString();
}

void QtEventView::disableSliceTypeSelection() {
  m_sliceTypeRadioButtons.disable();
}

void QtEventView::enableSliceTypeSelection() {
  m_sliceTypeRadioButtons.enable();
}

void QtEventView::onToggleUniform(bool isChecked) {
  if (isChecked) {
    Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
        "Feature", "ISIS Reflectometry->EventTab->EnableUniformSlicing", false);
    m_notifyee->notifySliceTypeChanged(SliceType::Uniform);
  }
}

void QtEventView::onToggleUniformEven(bool isChecked) {
  if (isChecked) {
    Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
        "Feature", "ISIS Reflectometry->EventTab->EnableUniformEvenSlicing",
        false);
    m_notifyee->notifySliceTypeChanged(SliceType::UniformEven);
  }
}

void QtEventView::onToggleCustom(bool isChecked) {
  if (isChecked) {
    Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
        "Feature", "ISIS Reflectometry->EventTab->EnableCustomSlicing", false);
    m_notifyee->notifySliceTypeChanged(SliceType::Custom);
  }
}

void QtEventView::onToggleLogValue(bool isChecked) {
  if (isChecked) {
    Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
        "Feature", "ISIS Reflectometry->EventTab->EnableLogValueSlicing",
        false);
    m_notifyee->notifySliceTypeChanged(SliceType::LogValue);
  }
}

void QtEventView::onToggleDisabledSlicing(bool isChecked) {
  if (isChecked) {
    Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
        "Feature", "ISIS Reflectometry->EventTab->DisableSlicing", false);
    m_notifyee->notifySliceTypeChanged(SliceType::None);
  }
}

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
