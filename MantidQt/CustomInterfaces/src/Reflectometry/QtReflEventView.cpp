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

  if (m_sliceType == "UniformEven")
    values = m_ui.uniformEvenEdit->text().toStdString();
  else if (m_sliceType == "Uniform")
    values = m_ui.uniformEdit->text().toStdString();
  else if (m_sliceType == "Custom")
    values = m_ui.customEdit->text().toStdString();

  return values;
}

/** Returns the type of time slicing that was selected
* @return :: Time slicing type
*/
std::string QtReflEventView::getTimeSlicingType() const { return m_sliceType; }

/** Enable slicing option entries for checked button and disable all others.
*/
void QtReflEventView::toggleSlicingOptions() const {

  const auto checkedButton = m_ui.slicingOptionsButtonGroup->checkedButton();

  const std::vector<std::string> slicingTypes = {"UniformEven", "Uniform",
                                                 "Custom"};

  std::vector<bool> entriesEnabled(m_buttonList.size(), false);
  for (size_t i = 0; i < m_buttonList.size(); i++) {
    if (m_buttonList[i] == checkedButton) {
      m_sliceType = slicingTypes[i];
      entriesEnabled[i] = true;
      break;
    }
  }

  m_ui.uniformEvenEdit->setEnabled(entriesEnabled[0]);
  m_ui.uniformEdit->setEnabled(entriesEnabled[1]);
  m_ui.customEdit->setEnabled(entriesEnabled[2]);
}

} // namespace CustomInterfaces
} // namespace Mantid
