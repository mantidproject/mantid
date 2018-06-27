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
  registerEventWidgets();
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
  m_sliceTypeRadioButtons =
      makeQWidgetGroup(m_ui.uniformEvenButton, m_ui.uniformButton,
                       m_ui.logValueButton, m_ui.customButton);
}

void QtReflEventTabView::initUniformSliceTypeLayout() {
  m_uniformGroup = makeQWidgetGroup(m_ui.uniformEdit, m_ui.uniformLabel);
  connect(m_ui.uniformButton, SIGNAL(toggled(bool)), this,
          SLOT(toggleUniform(bool)));
  connect(m_ui.);
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

std::string QtReflEventTabView::getLogValueTimeSlicingType() const {
  return textFrom(m_ui.logValueTypeEdit);
}

std::string QtReflEventTabView::getLogValueTimeSlicingValues() const {
  return textFrom(m_ui.logValueEdit);
}

std::string QtReflEventTabView::getCustomTimeSlicingValues() const {
  return textFrom(m_ui.customEdit);
}

std::string QtReflEventTabView::getUniformTimeSlicingValues() const {
  return textFrom(m_ui.uniformEdit);
}

std::string QtReflEventTabView::getUniformEvenTimeSlicingValues() const {
  return textFrom(m_ui.uniformEvenEdit);
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

void QtReflEventTabView::notifySettingsChanged() {
  m_notifyee->notifySettingsChanged();
}

void QtReflEventTabView::connectSettingsChange(QLineEdit &edit) {
  connect(&edit, SIGNAL(textChanged(QString const &)), this,
          SLOT(notifySettingsChanged()));
}

void QtReflEventTabView::connectSettingsChange(QGroupBox &edit) {
  connect(&edit, SIGNAL(toggled(bool)), this, SLOT(notifySettingsChanged()));
}

void QtReflEventTabView::registerEventWidgets() {
  connectSettingsChange(*m_ui.uniformGroup);
  connectSettingsChange(*m_ui.uniformEvenEdit);
  connectSettingsChange(*m_ui.uniformEdit);

  connectSettingsChange(*m_ui.customGroup);
  connectSettingsChange(*m_ui.customEdit);

  connectSettingsChange(*m_ui.logValueGroup);
  connectSettingsChange(*m_ui.logValueEdit);
  connectSettingsChange(*m_ui.logValueTypeEdit);
}
} // namespace CustomInterfaces
} // namespace Mantid
