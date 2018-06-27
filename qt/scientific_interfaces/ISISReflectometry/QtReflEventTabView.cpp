#include "QtReflEventTabView.h"
#include "ReflEventTabPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
* @param group :: [input] The group on the parent tab this belongs to
* @param parent :: [input] The parent of this widget
*/
QtReflEventTabView::QtReflEventTabView(QWidget *parent) {
  UNUSED_ARG(parent);
  initLayout();
}

void QtReflEventTabView::subscribe(EventTabViewSubscriber *notifyee) {
  m_notifyee = notifyee;
}

void QtReflEventTabView::initLayout() {
  m_ui.setupUi(this);
  initUniformSliceTypeLayout();
  initUniformEvenSliceTypeLayout();
  initLogValueSliceTypeLayout();
  initCustomSliceTypeLayout();
  connect(m_ui.disabledSlicingButton, SIGNAL(toggled(bool)), this,
          SLOT(toggleDisabledSlicing(bool)));

  m_sliceTypeRadioButtons = makeQWidgetGroup(
      m_ui.uniformEvenButton, m_ui.uniformButton, m_ui.logValueButton,
      m_ui.customButton, m_ui.disabledSlicingButton);
}

void QtReflEventTabView::initUniformSliceTypeLayout() {
  m_uniformGroup = makeQWidgetGroup(m_ui.uniformEdit, m_ui.uniformLabel);
  connect(m_ui.uniformButton, SIGNAL(toggled(bool)), this,
          SLOT(toggleUniform(bool)));

  connect(m_ui.uniformEvenEdit, SIGNAL(valueChanged(int)), this,
          SLOT(onUniformEvenChanged(int)));
  connect(m_ui.uniformEdit, SIGNAL(valueChanged(double)), this,
          SLOT(onUniformSecondsChanged(double)));

  connect(m_ui.customEdit, SIGNAL(textEdited(QString const &)), this,
          SLOT(onCustomChanged(QString const &)));
  connect(m_ui.logValueEdit, SIGNAL(textEdited(QString const &)), this,
          SLOT(onLogValuesChanged(QString const &)));
  connect(m_ui.logValueTypeEdit, SIGNAL(textEdited(QString const &)), this,
          SLOT(onLogValueTypeChanged(QString const &)));
}

void QtReflEventTabView::onUniformEvenChanged(int numberOfSlices) {
  m_notifyee->notifyUniformSliceCountChanged(numberOfSlices);
}

void QtReflEventTabView::onUniformSecondsChanged(double numberOfSeconds) {
  m_notifyee->notifyUniformSecondsChanged(numberOfSeconds);
}

void QtReflEventTabView::onCustomChanged(QString const &listOfSlices) {
  m_notifyee->notifyCustomSliceValuesChanged(listOfSlices.toStdString());
}

void QtReflEventTabView::onLogValuesChanged(
    QString const &listOfSliceBreakpoints) {
  m_notifyee->notifyLogSliceBreakpointsChanged(
      listOfSliceBreakpoints.toStdString());
}

void QtReflEventTabView::onLogValueTypeChanged(QString const &logBlockName) {
  m_notifyee->notifyLogBlockNameChanged(logBlockName.toStdString());
}

void QtReflEventTabView::initUniformEvenSliceTypeLayout() {
  m_uniformEvenGroup =
      makeQWidgetGroup(m_ui.uniformEvenEdit, m_ui.uniformEvenLabel);
  connect(m_ui.uniformEvenButton, SIGNAL(toggled(bool)), this,
          SLOT(toggleUniformEven(bool)));
}

void QtReflEventTabView::initCustomSliceTypeLayout() {
  m_customGroup = makeQWidgetGroup(m_ui.customEdit, m_ui.customLabel);
  connect(m_ui.customButton, SIGNAL(toggled(bool)), this,
          SLOT(toggleCustom(bool)));
}

void QtReflEventTabView::initLogValueSliceTypeLayout() {
  m_logValueGroup =
      makeQWidgetGroup(m_ui.logValueTypeEdit, m_ui.logValueTypeLabel,
                       m_ui.logValueEdit, m_ui.logValueLabel);
  connect(m_ui.logValueButton, SIGNAL(toggled(bool)), this,
          SLOT(toggleLogValue(bool)));
}

void QtReflEventTabView::enableSliceType(SliceType sliceType) {
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
  }
}

void QtReflEventTabView::disableSliceType(SliceType sliceType) {
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
  }
}

std::string QtReflEventTabView::logBlockName() const {
  return textFrom(m_ui.logValueTypeEdit);
}

std::string QtReflEventTabView::logBreakpoints() const {
  return textFrom(m_ui.logValueEdit);
}

std::string QtReflEventTabView::customBreakpoints() const {
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

void QtReflEventTabView::showCustomBreakpointsInvalid() {
  showAsInvalid(*m_ui.customEdit);
}

void QtReflEventTabView::showCustomBreakpointsValid() {
  showAsValid(*m_ui.customEdit);
}

void QtReflEventTabView::showLogBreakpointsInvalid() {
  showAsInvalid(*m_ui.logValueEdit);
}

void QtReflEventTabView::showLogBreakpointsValid() {
  showAsValid(*m_ui.logValueEdit);
}

int QtReflEventTabView::uniformSliceCount() const {
  return m_ui.uniformEvenEdit->value();
}

double QtReflEventTabView::uniformSliceLength() const {
  return m_ui.uniformEdit->value();
}

std::string QtReflEventTabView::textFrom(QLineEdit const *const widget) const {
  return widget->text().toStdString();
}

void QtReflEventTabView::disableSliceTypeSelection() {
  m_sliceTypeRadioButtons.disable();
}

void QtReflEventTabView::enableSliceTypeSelection() {
  m_sliceTypeRadioButtons.enable();
}

void QtReflEventTabView::toggleUniform(bool isChecked) {
  if (isChecked)
    m_notifyee->notifySliceTypeChanged(SliceType::Uniform);
}

void QtReflEventTabView::toggleUniformEven(bool isChecked) {
  if (isChecked)
    m_notifyee->notifySliceTypeChanged(SliceType::UniformEven);
}

void QtReflEventTabView::toggleCustom(bool isChecked) {
  if (isChecked)
    m_notifyee->notifySliceTypeChanged(SliceType::Custom);
}

void QtReflEventTabView::toggleLogValue(bool isChecked) {
  if (isChecked)
    m_notifyee->notifySliceTypeChanged(SliceType::LogValue);
}

void QtReflEventTabView::toggleDisabledSlicing(bool isChecked) {
  if (isChecked)
    m_notifyee->notifySliceTypeChanged(SliceType::None);
}

} // namespace CustomInterfaces
} // namespace Mantid
