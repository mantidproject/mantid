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

  // Add radio button / entry widget pairs
  m_buttonEntryPairsList.push_back(ButtonEntryPair(
      m_ui.uniformEvenTimeSlicesRadioButton, m_ui.uniformEvenTimeSlicesEdit));
  m_buttonEntryPairsList.push_back(ButtonEntryPair(
      m_ui.uniformTimeSlicesRadioButton, m_ui.uniformTimeSlicesEdit));
  m_buttonEntryPairsList.push_back(ButtonEntryPair(
      m_ui.customTimeSlicesRadioButton, m_ui.customTimeSlicesEdit));
  m_buttonEntryPairsList.push_back(
      ButtonEntryPair(m_ui.logValueSlicesRadioButton, m_ui.logValueComboBox));

  // Whenever one of the slicing options button is selected, their corresponding
  // entry is enabled, otherwise they remain disabled.
  for (auto &buttonEntryPair : m_buttonEntryPairsList) {
    connect(buttonEntryPair.first, SIGNAL(toggled(bool)), this,
            SLOT(toggleSlicingOptions()));
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

  return m_ui.customTimeSlicesEdit->text().toStdString();
}

/** Enable slicing option entry for checked button and disable all others.
*/
void QtReflEventView::toggleSlicingOptions() const {

  const auto checkedButton = m_ui.slicingOptionsButtonGroup->checkedButton();

  for (auto &buttonEntryPair : m_buttonEntryPairsList) {
    bool enable = buttonEntryPair.first == checkedButton;
    buttonEntryPair.second->setEnabled(enable);
  }
}

} // namespace CustomInterfaces
} // namespace Mantid
