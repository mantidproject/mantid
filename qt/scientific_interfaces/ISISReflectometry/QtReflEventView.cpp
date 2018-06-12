#include "QtReflEventView.h"
#include "ReflEventPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
* @param group :: [input] The group on the parent tab this belongs to
* @param parent :: [input] The parent of this widget
*/
QtReflEventView::QtReflEventView(int group, QWidget *parent) {
  UNUSED_ARG(parent);
  initLayout();
  m_presenter.reset(new ReflEventPresenter(this, group));
  registerEventWidgets();
}

QtReflEventView::~QtReflEventView() {}

void QtReflEventView::initLayout() {
  m_ui.setupUi(this);
  initUniformSliceTypeLayout();
  initUniformEvenSliceTypeLayout();
  initLogValueSliceTypeLayout();
  initCustomSliceTypeLayout();
  m_sliceTypeRadioButtons =
      makeQWidgetGroup(m_ui.uniformEvenButton, m_ui.uniformButton,
                       m_ui.logValueButton, m_ui.customButton);
}

void QtReflEventView::initUniformSliceTypeLayout() {
  m_uniformGroup = makeQWidgetGroup(m_ui.uniformEdit, m_ui.uniformLabel);
  connect(m_ui.uniformButton, SIGNAL(toggled(bool)), this,
          SLOT(toggleUniform(bool)));
}

void QtReflEventView::initUniformEvenSliceTypeLayout() {
  m_uniformEvenGroup =
      makeQWidgetGroup(m_ui.uniformEvenEdit, m_ui.uniformEvenLabel);
  connect(m_ui.uniformEvenButton, SIGNAL(toggled(bool)), this,
          SLOT(toggleUniformEven(bool)));
}

void QtReflEventView::initCustomSliceTypeLayout() {
  m_customGroup = makeQWidgetGroup(m_ui.customEdit, m_ui.customLabel);
  connect(m_ui.customButton, SIGNAL(toggled(bool)), this,
          SLOT(toggleCustom(bool)));
}

void QtReflEventView::initLogValueSliceTypeLayout() {
  m_logValueGroup =
      makeQWidgetGroup(m_ui.logValueTypeEdit, m_ui.logValueTypeLabel,
                       m_ui.logValueEdit, m_ui.logValueLabel);
  connect(m_ui.logValueButton, SIGNAL(toggled(bool)), this,
          SLOT(toggleLogValue(bool)));
}

/** Returns the presenter managing this view
* @return :: A pointer to the presenter
*/
IReflEventPresenter *QtReflEventView::getPresenter() const {
  return m_presenter.get();
}

void QtReflEventView::enableSliceType(SliceType sliceType) {
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

void QtReflEventView::disableSliceType(SliceType sliceType) {
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

std::string QtReflEventView::getLogValueTimeSlicingType() const {
  return textFrom(m_ui.logValueTypeEdit);
}

std::string QtReflEventView::getLogValueTimeSlicingValues() const {
  return textFrom(m_ui.logValueEdit);
}

std::string QtReflEventView::getCustomTimeSlicingValues() const {
  return textFrom(m_ui.customEdit);
}

std::string QtReflEventView::getUniformTimeSlicingValues() const {
  return textFrom(m_ui.uniformEdit);
}

std::string QtReflEventView::getUniformEvenTimeSlicingValues() const {
  return textFrom(m_ui.uniformEvenEdit);
}

std::string QtReflEventView::textFrom(QLineEdit const *const widget) const {
  return widget->text().toStdString();
}

void QtReflEventView::disableSliceTypeSelection() {
  m_sliceTypeRadioButtons.disable();
}

void QtReflEventView::enableSliceTypeSelection() {
  m_sliceTypeRadioButtons.enable();
}

void QtReflEventView::toggleUniform(bool isChecked) {
  if (isChecked)
    m_presenter->notifySliceTypeChanged(SliceType::Uniform);
}

void QtReflEventView::toggleUniformEven(bool isChecked) {
  if (isChecked)
    m_presenter->notifySliceTypeChanged(SliceType::UniformEven);
}

void QtReflEventView::toggleCustom(bool isChecked) {
  if (isChecked)
    m_presenter->notifySliceTypeChanged(SliceType::Custom);
}

void QtReflEventView::toggleLogValue(bool isChecked) {
  if (isChecked)
    m_presenter->notifySliceTypeChanged(SliceType::LogValue);
}

void QtReflEventView::notifySettingsChanged() {
  m_presenter->notifySettingsChanged();
}

void QtReflEventView::connectSettingsChange(QLineEdit &edit) {
  connect(&edit, SIGNAL(textChanged(QString const &)), this,
          SLOT(notifySettingsChanged()));
}

void QtReflEventView::connectSettingsChange(QGroupBox &edit) {
  connect(&edit, SIGNAL(toggled(bool)), this, SLOT(notifySettingsChanged()));
}

void QtReflEventView::registerEventWidgets() {
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
