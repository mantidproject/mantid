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

/** Returns the number of time slices
* @return :: The number of time slices
*/
std::string QtReflEventView::getTimeSlices() const {

  return m_ui.customEdit->text().toStdString();
}

/** Returns the time slicing value(s) obtained from the selected widget
* @return :: Time slicing values
*/
std::string QtReflEventView::getTimeSlicingValues() const {

  std::string values;
  const auto checkedButton = m_ui.slicingOptionsButtonGroup->checkedButton();

  if (checkedButton == m_buttonList[0]) {
    values = m_ui.uniformEvenEdit->text().toStdString();
  } else if (checkedButton == m_buttonList[1]) {
    values = m_ui.uniformEdit->text().toStdString();
  } else if (checkedButton == m_buttonList[2]) {
    values = m_ui.customEdit->text().toStdString();
  } else if (checkedButton == m_buttonList[3]) {
    values = m_ui.logValueComboBox->currentText().toStdString();
  }

  return values;
}

/** Returns the type of time slicing that was selected 
* @return :: Time slicing type
*/
std::string QtReflEventView::getTimeSlicingType() const {

  std::string type;
  const auto checkedButton = m_ui.slicingOptionsButtonGroup->checkedButton();

  if (checkedButton == m_buttonList[0]) {
    type = "UniformEven";
  } else if (checkedButton == m_buttonList[1]) {
    type = "Uniform";
  } else if (checkedButton == m_buttonList[2]) {
    type = "Custom";
  } else if (checkedButton == m_buttonList[3]) {
    type = "LogValue";
  }

  return type;
}

/** Enable slicing option entries for checked button and disable all others.
*/
void QtReflEventView::toggleSlicingOptions() const {

  const auto checkedButton = m_ui.slicingOptionsButtonGroup->checkedButton();

  std::vector<bool> entriesEnabled(m_buttonList.size(), false);
  for (size_t i = 0; i < m_buttonList.size(); i++) {
    if (m_buttonList[i] == checkedButton)
      entriesEnabled[i] = true;
  }

  m_ui.uniformEvenEdit->setEnabled(entriesEnabled[0]);
  m_ui.uniformEdit->setEnabled(entriesEnabled[1]);
  m_ui.customEdit->setEnabled(entriesEnabled[2]);
  m_ui.logValueComboBox->setEnabled(entriesEnabled[3]);
}

} // namespace CustomInterfaces
} // namespace Mantid
