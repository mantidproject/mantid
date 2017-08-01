#include "MantidQtCustomInterfaces/Reflectometry/QtReflEventView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflEventPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
* @param parent :: [input] The parent of this widget
*/
QtReflEventView::QtReflEventView(QWidget *parent) {

  UNUSED_ARG(parent);
  initLayout();

  // Insert slice-type to string pairs
  m_sliceTypeMap[SliceType::UniformEven] = "UniformEven";
  m_sliceTypeMap[SliceType::Uniform] = "Uniform";
  m_sliceTypeMap[SliceType::Custom] = "Custom";
  m_sliceTypeMap[SliceType::LogValue] = "LogValue";

  // Add slicing option buttons to list
  m_buttonList.push_back(m_ui.uniformEvenButton);
  m_buttonList.push_back(m_ui.uniformButton);
  m_buttonList.push_back(m_ui.customButton);
  m_buttonList.push_back(m_ui.logValueButton);

  // Whenever one of the slicing option buttons is selected, their corresponding
  // entry is enabled, otherwise they remain disabled.
  for (auto &button : m_buttonList) {
    connect(button, SIGNAL(toggled(bool)), this, SLOT(toggleSlicingOptions()));
  }

  toggleSlicingOptions(); // Run at least once

  m_presenter.reset(new ReflEventPresenter(this));
}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
QtReflEventView::~QtReflEventView() {}

/**
Initialise the Interface
*/
void QtReflEventView::initLayout() { m_ui.setupUi(this); }

/** Returns the presenter managing this view
* @return :: A pointer to the presenter
*/
IReflEventPresenter *QtReflEventView::getPresenter() const {

  return m_presenter.get();
}

/** Returns the time slicing value(s) obtained from the selected widget
* @return :: Time slicing values
*/
std::string QtReflEventView::getTimeSlicingValues() const {

  std::string values;

  switch (m_sliceType) {
  case SliceType::UniformEven:
    values = m_ui.uniformEvenEdit->text().toStdString();
    break;
  case SliceType::Uniform:
    values = m_ui.uniformEdit->text().toStdString();
    break;
  case SliceType::Custom:
    values = m_ui.customEdit->text().toStdString();
    break;
  case SliceType::LogValue:
    std::string slicingValues = m_ui.logValueEdit->text().toStdString();
    std::string logFilter = m_ui.logValueTypeEdit->text().toStdString();
    if (!slicingValues.empty() && !logFilter.empty())
      values = "Slicing=\"" + slicingValues + "\",LogFilter=" + logFilter;
    break;
  }

  return values;
}

/** Returns the type of time slicing that was selected as string
* @return :: Time slicing type
*/
std::string QtReflEventView::getTimeSlicingType() const {

  return m_sliceTypeMap.at(m_sliceType);
}

/** Enable slicing option entries for checked button and disable all others.
*/
void QtReflEventView::toggleSlicingOptions() const {

  const auto checkedButton = m_ui.slicingOptionsButtonGroup->checkedButton();

  SliceType slicingTypes[4] = {SliceType::UniformEven, SliceType::Uniform,
                               SliceType::Custom, SliceType::LogValue};

  std::vector<bool> entriesEnabled(m_buttonList.size(), false);
  for (size_t i = 0; i < m_buttonList.size(); i++) {
    if (m_buttonList[i] == checkedButton) {
      m_sliceType = slicingTypes[i];
      entriesEnabled[i] = true;
      break;
    }
  }

  // UniformEven
  m_ui.uniformEvenEdit->setEnabled(entriesEnabled[0]);
  m_ui.uniformEvenLabel->setEnabled(entriesEnabled[0]);
  // Uniform
  m_ui.uniformEdit->setEnabled(entriesEnabled[1]);
  m_ui.uniformLabel->setEnabled(entriesEnabled[1]);
  // Custom
  m_ui.customEdit->setEnabled(entriesEnabled[2]);
  m_ui.customLabel->setEnabled(entriesEnabled[2]);
  // LogValue
  m_ui.logValueEdit->setEnabled(entriesEnabled[3]);
  m_ui.logValueLabel->setEnabled(entriesEnabled[3]);
  m_ui.logValueTypeEdit->setEnabled(entriesEnabled[3]);
  m_ui.logValueTypeLabel->setEnabled(entriesEnabled[3]);
}

} // namespace CustomInterfaces
} // namespace Mantid
