#include "MantidQtCustomInterfaces/Reflectometry/QtReflSettingsTabView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflSettingsTabPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
* @param parent :: [input] The parent of this widget
*/
QtReflSettingsTabView::QtReflSettingsTabView(QWidget *parent) {

  initLayout();

  m_presenter = new ReflSettingsTabPresenter(this);
}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
QtReflSettingsTabView::~QtReflSettingsTabView() { delete m_presenter; }

/**
Initialise the Interface
*/
void QtReflSettingsTabView::initLayout() { m_ui.setupUi(this); }

/** Returns the presenter managing this view
* @return :: A pointer to the presenter
*/
IReflSettingsTabPresenter *QtReflSettingsTabView::getPresenter() const {

  return m_presenter;
}

/** Retuns the selected analysis mode
* @return :: Selected analysis mode
*/
std::string QtReflSettingsTabView::getAnalysisMode() const {

  return m_ui.analysisModeComboBox->currentText().toStdString();
}

/** Returns selected resolution (dQ/Q)
* @return :: Resolution as a string
*/
std::string QtReflSettingsTabView::getResolution() const {

  return m_ui.resolutionLineEdit->text().toStdString();
}

} // namespace CustomInterfaces
} // namespace Mantid
