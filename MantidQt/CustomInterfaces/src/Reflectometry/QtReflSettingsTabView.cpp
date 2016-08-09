#include "MantidQtCustomInterfaces/Reflectometry/QtReflSettingsTabView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflSettingsTabPresenter.h"
#include "MantidQtMantidWidgets/HintingLineEdit.h"

namespace MantidQt {
namespace CustomInterfaces {

using namespace MantidQt::MantidWidgets;

//----------------------------------------------------------------------------------------------
/** Constructor
* @param parent :: [input] The parent of this widget
*/
QtReflSettingsTabView::QtReflSettingsTabView(QWidget *parent) {

  UNUSED_ARG(parent);
  initLayout();

  m_presenter.reset(new ReflSettingsTabPresenter(this));
}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
QtReflSettingsTabView::~QtReflSettingsTabView() {}

/**
Initialise the Interface
*/
void QtReflSettingsTabView::initLayout() { m_ui.setupUi(this); }

/** Returns the presenter managing this view
* @return :: A pointer to the presenter
*/
IReflSettingsTabPresenter *QtReflSettingsTabView::getPresenter() const {

  return m_presenter.get();
}

/** Returns global options for 'Plus' algorithm
* @return :: Global options for 'Plus' algorithm
*/
std::string QtReflSettingsTabView::getPlusOptions() const {

  auto widget = m_ui.optionsLayout->itemAtPosition(1, 2)->widget();
  return static_cast<HintingLineEdit *>(widget)->text().toStdString();
}

/** Returns global options for 'CreateTransmissionWorkspaceAuto'
* @return :: Global options for 'CreateTransmissionWorkspaceAuto'
*/
std::string QtReflSettingsTabView::getTransmissionOptions() const {

  auto widget = m_ui.optionsLayout->itemAtPosition(2, 2)->widget();
  return static_cast<HintingLineEdit *>(widget)->text().toStdString();
}

/** Returns global options for 'ReflectometryReductionOneAuto'
* @return :: Global options for 'ReflectometryReductionOneAuto'
*/
std::string QtReflSettingsTabView::getReductionOptions() const {

  auto widget = m_ui.optionsLayout->itemAtPosition(3, 2)->widget();
  return static_cast<HintingLineEdit *>(widget)->text().toStdString();
}

/** Returns global options for 'Stitch1DMany'
* @return :: Global options for 'Stitch1DMany'
*/
std::string QtReflSettingsTabView::getStitchOptions() const {

  auto widget = m_ui.optionsLayout->itemAtPosition(4, 2)->widget();
  return static_cast<HintingLineEdit *>(widget)->text().toStdString();
}

/** Creates hints for 'Plus'
* @param hints :: Hints as a map
*/
void QtReflSettingsTabView::createPlusHints(
    const std::map<std::string, std::string> &hints) {

  m_ui.optionsLayout->addWidget(new HintingLineEdit(this, hints), 1, 2);
}

/** Creates hints for 'CreateTransmissionWorkspaceAuto'
* @param hints :: Hints as a map
*/
void QtReflSettingsTabView::createTransmissionHints(
    const std::map<std::string, std::string> &hints) {

  m_ui.optionsLayout->addWidget(new HintingLineEdit(this, hints), 2, 2);
}

/** Creates hints for 'ReflectometryReductionOneAuto'
* @param hints :: Hints as a map
*/
void QtReflSettingsTabView::createReductionHints(
    const std::map<std::string, std::string> &hints) {

  m_ui.optionsLayout->addWidget(new HintingLineEdit(this, hints), 3, 2);
}

/** Creates hints for 'Stitch1DMany'
* @param hints :: Hints as a map
*/
void QtReflSettingsTabView::createStitchHints(
    const std::map<std::string, std::string> &hints) {

  m_ui.optionsLayout->addWidget(new HintingLineEdit(this, hints), 4, 2);
}

} // namespace CustomInterfaces
} // namespace Mantid
