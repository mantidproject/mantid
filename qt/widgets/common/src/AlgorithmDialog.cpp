// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/DateAndTimeHelpers.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/Logger.h"
#include <csignal>

#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"
#include "MantidQtWidgets/Common/FilePropertyWidget.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Common/MantidWidget.h"
#include "MantidQtWidgets/Common/PropertyWidget.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

#include <Poco/ActiveResult.h>

using namespace MantidQt::API;
using namespace Mantid::Kernel::DateAndTimeHelpers;
using Mantid::API::IAlgorithm;
using Mantid::Types::Core::DateAndTime;

namespace {
Mantid::Kernel::Logger g_log("AlgorithmDialog");
}

//------------------------------------------------------
// Public member functions
//------------------------------------------------------
/**
 * Default Constructor
 */
AlgorithmDialog::AlgorithmDialog(QWidget *parent)
    : QDialog(parent), m_algorithm(), m_algName(""), m_algProperties(), m_propertyValueMap(), m_tied_properties(),
      m_forScript(false), m_python_arguments(), m_enabled(), m_disabled(), m_strMessage(""), m_keepOpen(false),
      m_msgAvailable(false), m_isInitialized(false), m_autoParseOnInit(true), m_validators(), m_noValidation(),
      m_inputws_opts(), m_outputws_fields(), m_wsbtn_tracker(), m_keepOpenCheckBox(nullptr), m_okButton(nullptr),
      m_exitButton(nullptr), m_observers(), m_btnTimer(), m_statusTracked(false) {
  m_btnTimer.setSingleShot(true);
}

/**
 * Destructor
 */
AlgorithmDialog::~AlgorithmDialog() {
  m_observers.clear();
  if (m_statusTracked) {
    this->stopObserving(m_algorithm);
  }
}

/**
 * Set if the keep open option is shown.
 * This must be set after calling initializeLayout.
 * @param showOption false to hide the control, otherwise true
 */
void AlgorithmDialog::setShowKeepOpen(const bool showOption) {
  if (m_keepOpenCheckBox) {
    // if hidden then turn it off
    if (!showOption) {
      m_keepOpenCheckBox->setCheckState(Qt::CheckState::Unchecked);
    }
    m_keepOpenCheckBox->setVisible(showOption);
  }
}
/**
 * Is the keep open option going to be shown?
 * @returns true if it will be shown
 */
bool AlgorithmDialog::isShowKeepOpen() const {
  bool retval = true;
  if (m_keepOpenCheckBox) {
    retval = m_keepOpenCheckBox->isVisible();
  }
  return retval;
}

/**
 * Create the layout for this dialog.
 *
 * The default is to execute the algorithm when accept() is called. This
 * assumes that the AlgorithmManager owns the
 * algorithm pointer as it must survive after the dialog is destroyed.
 */
void AlgorithmDialog::initializeLayout() {
  if (isInitialized())
    return;

  // Set a common title
  setWindowTitle(QString::fromStdString(getAlgorithm()->name()) + " input dialog");
  // Set the icon
  setWindowIcon(QIcon(":/mantidplot.png"));

  // These containers are for ensuring the 'replace input workspace; button
  // works correctly
  // Store all combo boxes that relate to an input workspace
  m_inputws_opts.clear();
  // Store all line edit fields that relate to an output workspace name
  m_outputws_fields.clear();
  // Keep track of the input workspace that has been used to fill the output
  // workspace. Each button click
  // cycles through all of the input workspaces
  m_wsbtn_tracker.clear();

  // This derived class function creates the layout of the widget. It can also
  // add default input if the
  // dialog has been written this way
  this->initLayout();

  if (m_autoParseOnInit) {
    // Check if there is any default input
    this->parse();
    // Unless told not to, try to set these values. This will validate the
    // defaults and mark those that are invalid, if any.
    this->setPropertyValues();
  }

  executeOnAccept(true);

  connect(this, SIGNAL(algCompletedSignal()), this, SLOT(algorithmCompleted()));

  m_isInitialized = true;
}

/**
 * Has this dialog been initialized yet
 *  @returns Whether initialzedLayout has been called yet
 */
bool AlgorithmDialog::isInitialized() const { return m_isInitialized; }

//------------------------------------------------------
// Protected member functions
//------------------------------------------------------
/**
 * Parse input from widgets on the dialog. This function does nothing in the
 * base class
 */
void AlgorithmDialog::parseInput() {}

//-------------------------------------------------------------------------------------------------
/**
 * Save the property values to the input history
 */
void AlgorithmDialog::saveInput() {
  AlgorithmInputHistory::Instance().clearAlgorithmInput(m_algName);
  QStringList::const_iterator pend = m_algProperties.end();
  for (QStringList::const_iterator pitr = m_algProperties.begin(); pitr != pend; ++pitr) {
    const Mantid::Kernel::Property *p = getAlgorithmProperty(*pitr);
    if (p->remember()) {
      QString pName = *pitr;
      QString value = m_propertyValueMap.value(pName);
      AlgorithmInputHistory::Instance().storeNewValue(m_algName, QPair<QString, QString>(pName, value));
    }
  }
}

//-------------------------------------------------------------------------------------------------
/**
 * Set the algorithm pointer
 * @param alg :: A pointer to the algorithm
 */
void AlgorithmDialog::setAlgorithm(const Mantid::API::IAlgorithm_sptr &alg) {
  m_algorithm = alg;
  m_algName = QString::fromStdString(alg->name());
  m_algProperties.clear();
  m_tied_properties.clear();
  std::vector<Mantid::Kernel::Property *>::const_iterator iend = alg->getProperties().end();
  for (std::vector<Mantid::Kernel::Property *>::const_iterator itr = alg->getProperties().begin(); itr != iend; ++itr) {
    Mantid::Kernel::Property *p = *itr;
    if (dynamic_cast<Mantid::API::IWorkspaceProperty *>(p) || p->direction() != Mantid::Kernel::Direction::Output) {
      m_algProperties.append(QString::fromStdString(p->name()));
    }
  }

  m_validators.clear();
  m_noValidation.clear();
}

/**
 * Get the algorithm pointer
 * @returns A pointer to the algorithm that is associated with the dialog
 */
Mantid::API::IAlgorithm_sptr AlgorithmDialog::getAlgorithm() const { return m_algorithm; }

/**
 * Get a named property for this algorithm
 * @param propName :: The name of the property
 */
const Mantid::Kernel::Property *AlgorithmDialog::getAlgorithmProperty(const QString &propName) const {
  if (m_algProperties.contains(propName)) {
    return m_algorithm->getProperty(propName.toStdString());
  } else
    return nullptr;
}

/**
 * Return a true if the given property requires user input
 * @param propName :: The name of the property
 */
bool AlgorithmDialog::requiresUserInput(const QString &propName) const { return m_algProperties.contains(propName); }

//-------------------------------------------------------------------------------------------------
/**
 * Get an input value from the form, dealing with blank inputs etc
 * @param propName :: The name of the property
 */
QString AlgorithmDialog::getInputValue(const QString &propName) const {
  QString value = m_propertyValueMap.value(propName);
  if (value.isEmpty()) {
    const Mantid::Kernel::Property *prop = getAlgorithmProperty(propName);
    if (prop)
      return QString::fromStdString(prop->getDefault());
    else
      return "";
  }

  return value;
}

//-------------------------------------------------------------------------------------------------
/** Get or make a property validator label (that little red star)
 *
 * @param propname :: name of the Property
 * @return the QLabel pointer. Will create one if needed.
 */
QLabel *AlgorithmDialog::getValidatorMarker(const QString &propname) {
  if (m_noValidation.contains(propname))
    return nullptr;
  QLabel *validLbl(nullptr);
  if (!m_validators.contains(propname)) {
    validLbl = new QLabel("*", this);
    QPalette pal = validLbl->palette();
    pal.setColor(QPalette::WindowText, Qt::darkRed);
    validLbl->setPalette(pal);
    validLbl->setVisible(true);
    m_validators[propname] = validLbl;
  } else {
    validLbl = m_validators.value(propname);
  }
  return validLbl;
}

//-------------------------------------------------------------------------------------------------
/**
 * Adds a property (name,value) pair to the stored map
 */
void AlgorithmDialog::storePropertyValue(const QString &name, const QString &value) {
  if (name.isEmpty())
    return;
  m_propertyValueMap.insert(name, value);
}

//-------------------------------------------------------------------------------------------------
/**
 * Adds a property (name,value) pair to the stored map.
 * @param name :: The name of the property.
 */
void AlgorithmDialog::removePropertyValue(const QString &name) {
  if (name.isEmpty())
    return;
  m_propertyValueMap.remove(name);
}

//-------------------------------------------------------------------------------------------------
/** Show the validators for all the properties */
void AlgorithmDialog::showValidators() {
  // Do nothing for non-generic algorithm dialogs
  QStringList::const_iterator pend = m_algProperties.end();
  for (QStringList::const_iterator pitr = m_algProperties.begin(); pitr != pend; ++pitr) {
    const QString propName = *pitr;

    // Find the widget for this property.
    if (m_tied_properties.contains(propName)) {
      // Show/hide the validator label (that red star)
      QString error = "";
      if (m_errors.contains(propName))
        error = m_errors[propName];

      QLabel *validator = getValidatorMarker(propName);
      // If there's no validator then assume it's handling its own validation
      // notification
      if (validator && validator->parent()) {
        validator->setToolTip(error);
        validator->setVisible(error.length() != 0);
      }
    } // widget is tied
  } // for each property
}

//-------------------------------------------------------------------------------------------------
/** Sets the value of a single property, using the value previously stored using
 * storePropertyValue()
 *
 * @param pName :: name of the property to set
 * @param validateOthers :: set to true to validate, enable, or hide ALL other
 * properties after.
 *        Set false if you are setting ALL property values and do it once at the
 * end.
 * @return true if the property is valid.
 */
bool AlgorithmDialog::setPropertyValue(const QString &pName, bool validateOthers) {
  // Mantid::Kernel::Property *p = getAlgorithmProperty(pName);
  QString value = getInputValue(pName);

  std::string error("");
  try {
    // error = p->setValue(value.toStdString());
    getAlgorithm()->setPropertyValue(pName.toStdString(), value.toStdString());
  } catch (std::exception &err_details) {
    error = err_details.what();
  }
  // Save the error string for later
  m_errors[pName] = QString::fromStdString(error).trimmed();

  // Go through all the other properties' validators
  if (validateOthers)
    this->showValidators();

  // Prop was valid if the error string is empty
  return error.empty();
}

//-------------------------------------------------------------------------------------------------
/** Set the properties that have been parsed from the dialog.
 *
 * @param skipList :: An optional list of property names whose values will not
 * be set
 * @returns A boolean that indicates if the validation was successful.
 */
bool AlgorithmDialog::setPropertyValues(const QStringList &skipList) {
  QStringList::const_iterator pend = m_algProperties.end();
  bool allValid(true);
  for (QStringList::const_iterator pitr = m_algProperties.begin(); pitr != pend; ++pitr) {
    const QString pName = *pitr;
    if (skipList.contains(pName)) {
      // For the load dialog, skips setting some properties
      const Mantid::Kernel::Property *p = getAlgorithmProperty(pName);
      std::string error = p->isValid();
      m_errors[pName] = QString::fromStdString(error).trimmed();
      if (!error.empty())
        allValid = false;
    } else {
      bool thisValid = this->setPropertyValue(pName, false);
      allValid = allValid && thisValid;
    }
  }

  // Do additional validation on the WHOLE set of properties
  // But only if the individual validation passed
  if (allValid) {
    std::map<std::string, std::string> errs = m_algorithm->validateInputs();
    for (auto &err : errs) {
      // only count as an error if the named property exists
      if (m_algorithm->existsProperty(err.first)) {
        const QString pName = QString::fromStdString(err.first);
        const QString value = QString::fromStdString(err.second);
        if (m_errors.contains(pName)) {
          if (!m_errors[pName].isEmpty())
            m_errors[pName] += "\n";
          m_errors[pName] += value;
        } else
          m_errors[pName] = value;
        // There is at least one whole-algo error
        allValid = false;
      }
    }
  }

  // OK all the values have been set once. Time to look for which should be
  // enabled
  this->showValidators();

  return allValid;
}

//-------------------------------------------------------------------------------------------------
/**
 * Return the message string
 * @returns the message string
 */
const QString &AlgorithmDialog::getOptionalMessage() const { return m_strMessage; }

//-------------------------------------------------------------------------------------------------
/** Add the optional message in a light yellow box to the layout
 *
 * @param mainLay :: layout
 */
void AlgorithmDialog::addOptionalMessage(QVBoxLayout *mainLay) {
  QLabel *inputMessage = new QLabel(this);
  inputMessage->setFrameStyle(static_cast<int>(QFrame::Panel) | static_cast<int>(QFrame::Sunken));
  QPalette pal = inputMessage->palette();
  pal.setColor(inputMessage->backgroundRole(), QColor(255, 255, 224)); // Light yellow
  pal.setColor(inputMessage->foregroundRole(), Qt::black);
  inputMessage->setPalette(pal);
  inputMessage->setAutoFillBackground(true);
  inputMessage->setWordWrap(true);
  inputMessage->setAlignment(Qt::AlignJustify);
  inputMessage->setMargin(3);
  inputMessage->setText(getOptionalMessage());
  auto *msgArea = new QHBoxLayout;
  msgArea->addWidget(inputMessage);
  mainLay->addLayout(msgArea, 0);
}

//-------------------------------------------------------------------------------------------------
/**
 * Was this dialog raised from a script? This is important when deciding what to
 * do with properties that have old input
 * @returns A boolean inidcating whether we are being called from a script
 */
bool AlgorithmDialog::isForScript() const { return m_forScript; }

/*
 * Is there a message string available
 * @returns A boolean indicating whether the message string is empty
 */
bool AlgorithmDialog::isMessageAvailable() const { return !m_strMessage.isEmpty(); }

//-------------------------------------------------------------------------------------------------
/**
 * Check if the control should be enabled for this property
 * @param propName :: The name of the property
 */
bool AlgorithmDialog::isWidgetEnabled(const QString &propName) const {
  // To avoid errors
  if (propName.isEmpty())
    return true;

  // Otherwise it must be disabled but only if it is valid
  const Mantid::Kernel::Property *property = getAlgorithmProperty(propName);
  if (!property)
    return true;

  if (!isForScript()) {
    // Regular C++ algo. Let the property tell us,
    // possibly using validators, if it is to be shown enabled
    if (property->getSettings())
      return property->getSettings()->isEnabled(getAlgorithm().get());
    else
      return true;
  } else {
    // Algorithm dialog was called from a script(i.e. Python)
    // Keep things enabled if requested
    if (m_enabled.contains(propName))
      return true;

    /**
     * The control is disabled if
     *   (1) It is contained in the disabled list or
     *   (2) A user passed a value into the dialog
     */

    return !(m_disabled.contains(propName) || m_python_arguments.contains(propName));
  }
}

//-------------------------------------------------------------------------------------------------
/**
 * UnTie a property
 * @param property :: The name of the property to tie the given widget to
 */
void AlgorithmDialog::untie(const QString &property) {
  if (m_tied_properties.contains(property)) {
    m_tied_properties.remove(property);
  }
}

//-------------------------------------------------------------------------------------------------
/**
 * Tie together an input widget and a property
 * @param widget :: The widget that will collect the input
 * @param property :: The name of the property to tie the given widget to
 * @param parent_layout :: An optional pointer to a QLayout class that is
 * reponsible for managing the passed widget.
 * If given, a validator label will be added for the given input widget
 * @param readHistory :: If true then a history value will be retrieved
 *
 * @return A NULL pointer if a valid label was successfully add to a passed
 * parent_layout otherwise it
 *          returns a pointer to the QLabel instance marking the validity
 */
QWidget *AlgorithmDialog::tie(QWidget *widget, const QString &property, QLayout *parent_layout, bool readHistory) {
  if (m_tied_properties.contains(property))
    m_tied_properties.remove(property);

  const Mantid::Kernel::Property *prop = getAlgorithmProperty(property);
  if (prop) { // Set a few things on the widget
    widget->setToolTip(QString::fromStdString(prop->documentation()));
  }
  widget->setEnabled(isWidgetEnabled(property));

  const PropertyWidget *propWidget = qobject_cast<const PropertyWidget *>(widget);

  // Save in the hashes
  m_tied_properties.insert(property, widget);

  // If the widget's layout has been given then assume that a validator is
  // required, else assume not
  QWidget *validlbl(nullptr);
  if (parent_layout) {
    // Check if the validator is already there
    validlbl = getValidatorMarker(property);
    if (validlbl) {
      // Find where it was sitting in the layout
      int item_index;
      if (propWidget)
        item_index = parent_layout->indexOf(propWidget->getMainWidget());
      else
        item_index = parent_layout->indexOf(widget);

      if (QBoxLayout *box = qobject_cast<QBoxLayout *>(parent_layout)) {
        box->insertWidget(item_index + 1, validlbl);
      } else if (QGridLayout *grid = qobject_cast<QGridLayout *>(parent_layout)) {
        int row(0), col(0), span(0);
        grid->getItemPosition(item_index, &row, &col, &span, &span);
        grid->addWidget(validlbl, row, col + 2);
      } else {
      }
    }
  } else {
    m_noValidation.append(property);
  }

  if (readHistory) {
    setPreviousValue(widget, property);
  }

  // If the widget is a line edit and has no value then set the placeholder text
  // to the default value.
  QLineEdit *textfield = qobject_cast<QLineEdit *>(widget);
  if ((textfield)) {
    if (prop) {
      PropertyWidget::setFieldPlaceholderText(prop, textfield);
    }
  }

  return validlbl;
}

//-------------------------------------------------------------------------------------------------
/**
 * Open a file selection box. The type of dialog, i.e. load/save will depend on
 * the
 * property type
 * @param propName :: The property name that this is associated with.
 */
QString AlgorithmDialog::openFileDialog(const QString &propName) {
  if (propName.isEmpty())
    return "";
  return FilePropertyWidget::openFileDialog(this->getAlgorithmProperty(propName));
}

//-------------------------------------------------------------------------------------------------
/**
 * Takes a combobox and adds the allowed values of the given property to its
 * list.
 * It also sets the displayed value to the correct one based on either the
 * history
 * or a script input value
 * @param propName :: The name of the property
 * @param optionsBox :: A pointer to a QComoboBox object
 */
void AlgorithmDialog::fillAndSetComboBox(const QString &propName, QComboBox *optionsBox) const {
  if (!optionsBox)
    return;
  const Mantid::Kernel::Property *property = getAlgorithmProperty(propName);
  if (!property)
    return;

  std::vector<std::string> items = property->allowedValues();
  std::vector<std::string>::const_iterator vend = items.end();
  for (std::vector<std::string>::const_iterator vitr = items.begin(); vitr != vend; ++vitr) {
    optionsBox->addItem(QString::fromStdString(*vitr));
  }

  // Display the appropriate value
  QString displayed("");
  if (!isForScript()) {
    displayed = AlgorithmInputHistory::Instance().previousInput(m_algName, propName);
  }
  if (displayed.isEmpty()) {
    displayed = QString::fromStdString(property->value());
  }

  int index = optionsBox->findText(displayed);
  if (index >= 0) {
    optionsBox->setCurrentIndex(index);
  }
}

//-------------------------------------------------------------------------------------------------
/**
 * Set the input for a text box based on either the history or a script value
 * @param propName :: The name of the property
 * @param textField :: The QLineEdit field
 */
void AlgorithmDialog::fillLineEdit(const QString &propName, QLineEdit *textField) {
  if (!isForScript()) {
    textField->setText(AlgorithmInputHistory::Instance().previousInput(m_algName, propName));
  } else {
    const Mantid::Kernel::Property *property = getAlgorithmProperty(propName);
    if (property && property->isValid().empty() && (m_python_arguments.contains(propName) || !property->isDefault())) {
      textField->setText(QString::fromStdString(property->value()));
    }
  }
}

//-------------------------------------------------------------------------------------------------
/** Layout the buttons and others in the generic dialog */
QLayout *AlgorithmDialog::createDefaultButtonLayout(const QString &helpText, const QString &loadText,
                                                    const QString &cancelText, const QString &keepOpenText) {
  m_okButton = new QPushButton(loadText);
  connect(m_okButton, SIGNAL(clicked()), this, SLOT(accept()));
  m_okButton->setDefault(true);

  m_exitButton = new QPushButton(cancelText);
  connect(m_exitButton, SIGNAL(clicked()), this, SLOT(reject()));

  auto *buttonRowLayout = new QHBoxLayout;
  buttonRowLayout->addWidget(createHelpButton(helpText));
  buttonRowLayout->addStretch();

  m_keepOpenCheckBox = new QCheckBox(keepOpenText);
  m_keepOpenCheckBox->setLayoutDirection(Qt::LayoutDirection::RightToLeft);
  connect(m_keepOpenCheckBox, SIGNAL(stateChanged(int)), this, SLOT(keepOpenChanged(int)));
  buttonRowLayout->addWidget(m_keepOpenCheckBox);
  if (keepOpenText.isEmpty()) {
    setShowKeepOpen(false);
  }

  buttonRowLayout->addWidget(m_okButton);
  buttonRowLayout->addWidget(m_exitButton);

  return buttonRowLayout;
}

//-------------------------------------------------------------------------------------------------
/**
 * Create a help button that, when clicked, will open a browser to the Mantid
 * wiki page
 * for that algorithm
 */
QPushButton *AlgorithmDialog::createHelpButton(const QString &helpText) const {
  auto *help = new QPushButton(helpText);
  connect(help, SIGNAL(clicked()), this, SLOT(helpClicked()));
  return help;
}

/**
 * Flag an input workspace widget
 * @param inputWidget :: A widget used to enter the input workspace
 */
void AlgorithmDialog::flagInputWS(QWidget *inputWidget) { m_inputws_opts.push_back(inputWidget); }

//-----------------------------------------------------------
// Protected slots
//-----------------------------------------------------------
/**
 * A slot that can be used to connect a button that accepts the dialog if
 * all of the properties are valid
 */
void AlgorithmDialog::accept() {
  // Get property values
  parse();

  // Try and set and validate the properties and
  if (setPropertyValues()) {
    // Store input for next time
    saveInput();
    if (!this->m_keepOpen) {
      QDialog::accept();
    } else {
      executeAlgorithmAsync();
    }
  } else {
    QMessageBox::critical(this, "",
                          "One or more properties are invalid. The invalid properties are\n"
                          "marked with a *, hold your mouse over the * for more information.");
  }
}

void AlgorithmDialog::reject() {
  emit closeEventCalled();
  QDialog::reject();
}

//-------------------------------------------------------------------------------------------------

/**
 * A slot to handle the help button click
 */
void AlgorithmDialog::helpClicked() {
  std::raise(SIGTRAP);
  // determine the version to show
  int version(-1); // the latest version
  if (m_algorithm)
    version = m_algorithm->version();

  // use interface manager instead of help window
  MantidQt::API::InterfaceManager().showAlgorithmHelp(m_algName, version);
}

//-------------------------------------------------------------------------------------------------

/**
 * A slot to handle the keep open button click
 */
void AlgorithmDialog::keepOpenChanged(int state) { m_keepOpen = (state == Qt::Checked); }

//-------------------------------------------------------------------------------------------------

/**
 * Execute the underlying algorithm
 */
void AlgorithmDialog::executeAlgorithmAsync() {
  Mantid::API::IAlgorithm_sptr algToExec = m_algorithm;
  // Clear any previous trackers so we know what state we are in
  this->stopObserving(algToExec);

  try {
    // Add any custom AlgorithmObservers to the algorithm
    for (auto &observer : m_observers) {
      observer->observeAll(algToExec);
    }

    // Only need to observe finish events if we are staying open
    if (m_keepOpenCheckBox && m_keepOpenCheckBox->isChecked()) {
      this->observeFinish(algToExec);
      this->observeError(algToExec);
      m_statusTracked = true;
      // Disable close button for a short period. If it is clicked to soon then
      // Mantid crashes - https://github.com/mantidproject/mantid/issues/13836
      if (m_exitButton) {
        m_exitButton->setEnabled(false);
        m_btnTimer.setInterval(1000);
        connect(&m_btnTimer, SIGNAL(timeout()), this, SLOT(enableExitButton()));
      }
    }
    algToExec->executeAsync();
    m_btnTimer.start();

    if (m_okButton) {
      m_okButton->setEnabled(false);
    }
  } catch (Poco::NoThreadAvailableException &) {
    g_log.error() << "No thread was available to run the " << algToExec->name() << " algorithm in the background.\n";
  }
}

//-------------------------------------------------------------------------------------------------

void AlgorithmDialog::removeAlgorithmFromManager() {
  using namespace Mantid::API;
  AlgorithmManager::Instance().removeById(m_algorithm->getAlgorithmID());
}

void AlgorithmDialog::enableExitButton() { m_exitButton->setEnabled(true); }

void AlgorithmDialog::disableExitButton() { m_exitButton->setEnabled(false); }

//------------------------------------------------------
// Private member functions
//------------------------------------------------------
/**
 * Parse out information from the dialog
 */
void AlgorithmDialog::parse() {
  QHashIterator<QString, QWidget *> itr(m_tied_properties);
  while (itr.hasNext()) {
    itr.next();
    // Need to do different things depending on the type of the widget. getValue
    // sorts this out
    storePropertyValue(itr.key(), getValue(itr.value()));
  }

  // Now call parseInput, which can be overridden in an inheriting class
  parseInput();
}

//-------------------------------------------------------------------------------------------------
/**
 * Set a list of values for the properties
 * @param presetValues :: A string containing a list of "name=value" pairs with
 * each separated by an '|' character
 */
void AlgorithmDialog::setPresetValues(const QHash<QString, QString> &presetValues) {
  if (presetValues.isEmpty())
    return;
  QHashIterator<QString, QString> itr(presetValues);
  m_python_arguments.clear();
  while (itr.hasNext()) {
    itr.next();
    QString name = itr.key();
    m_python_arguments.append(name);
    QString value = itr.value();
    storePropertyValue(name, value);
  }
  setPropertyValues();
}

//------------------------------------------------------------------------------------------------
/**
 * Set list of enabled and disabled parameter names
 * @param enabled:: A list of parameter names to keep enabled
 * @param disabled:: A list of parameter names whose widgets should be disabled
 */
void AlgorithmDialog::addEnabledAndDisableLists(const QStringList &enabled, const QStringList &disabled) {
  m_enabled = enabled;
  m_disabled = disabled;
}

//------------------------------------------------------------------------------------------------
/**
 * Returns true if the parameter name has been explicity requested to be kept
 * enabled. If the parameter
 * has been explicity requested to be disabled then return false as well as if
 * neither have been specified
 */
bool AlgorithmDialog::requestedToKeepEnabled(const QString &propName) const {
  bool enabled(true);
  if (m_disabled.contains(propName)) {
    enabled = false;
  } else if (m_enabled.contains(propName)) // Definitely enable
  {
    enabled = true;
  } else // Nothing was specified
  {
    enabled = false;
  }
  return enabled;
}

//------------------------------------------------------------------------------------------------
/** Set if we are for a script or not
 * @param forScript :: A boolean indicating whether we are being called from a
 * script */
void AlgorithmDialog::isForScript(bool forScript) { m_forScript = forScript; }

//------------------------------------------------------------------------------------------------
/**
 * @param on If true the algorithm is executed when "ok" is pressed
 */
void AlgorithmDialog::executeOnAccept(bool on) {
  if (on) {
    connect(this, SIGNAL(accepted()), this, SLOT(executeAlgorithmAsync()));
    connect(this, SIGNAL(rejected()), this, SLOT(removeAlgorithmFromManager()));
  } else {
    disconnect(this, SIGNAL(accepted()), this, SLOT(executeAlgorithmAsync()));
    disconnect(this, SIGNAL(rejected()), this, SLOT(removeAlgorithmFromManager()));
  }
}

//-------------------------------------------------------------------------------------------------
/** Set an optional message to be displayed at the top of the widget
 * @param message :: The message string */
void AlgorithmDialog::setOptionalMessage(const QString &message) {
  m_strMessage = message;
  if (message.isEmpty())
    m_strMessage = QString::fromStdString(getAlgorithm()->summary());
  if (m_strMessage.isEmpty())
    m_msgAvailable = false;
  else
    m_msgAvailable = true;
}

//-------------------------------------------------------------------------------------------------
/** Get a value from a widget.
 *
 * The function needs to know about the types of widgets
 * that are being used. Currently it knows about QComboBox, QLineEdit, QCheckBox
 * and FileFinderWidget
 * @param widget :: A pointer to the widget
 */
QString AlgorithmDialog::getValue(QWidget *widget) {
  if (QComboBox *opts = qobject_cast<QComboBox *>(widget)) {
    return opts->currentText().trimmed();
  } else if (QLineEdit *textfield = qobject_cast<QLineEdit *>(widget)) {
    return textfield->text().trimmed();
  } else if (QAbstractButton *checker = qobject_cast<QAbstractButton *>(widget)) {
    return checker->isChecked() ? QString("1") : QString("0");
  } else if (QDateTimeEdit *dateEdit = qobject_cast<QDateTimeEdit *>(widget)) {
    // String in ISO8601 format /* add toUTC() to go from local time */
    QString value = dateEdit->dateTime().toString(Qt::ISODate);
    return value;
  } else if (const MantidWidget *mtd_widget = qobject_cast<const MantidWidget *>(widget)) { // Changed here
    return mtd_widget->getUserInput().toString().trimmed();
  } else if (const PropertyWidget *propWidget = qobject_cast<const PropertyWidget *>(widget)) { // And here
    return propWidget->getValue().trimmed();
  } else {
    QMessageBox::warning(this, windowTitle(),
                         QString("Cannot parse input from ") + widget->metaObject()->className() +
                             ". Update AlgorithmDialog::getValue() to cope with this widget.");
    return "";
  }
}

QString AlgorithmDialog::getPreviousValue(const QString &propName) const {
  QString value;

  if (!isForScript()) {
    value = m_propertyValueMap.value(propName);
    if (value.isEmpty())
      value = AlgorithmInputHistory::Instance().previousInput(m_algName, propName);
  } else if (getAlgorithmProperty(propName)) {
    value = m_propertyValueMap.value(propName);
  }

  return value;
}

//------------------------------------------------------------------------------------------------
/** Set a value for a widget.
 *
 * The function needs to know about the types of widgets
 * that are being used. Currently it knows about QComboBox, QLineEdit and
 * QCheckBox
 * @param widget :: A pointer to the widget
 * @param propName :: The property name
 */
void AlgorithmDialog::setPreviousValue(QWidget *widget, const QString &propName) {
  // If is called from a script, check if we have such property
  if (isForScript() && !getAlgorithmProperty(propName))
    return;

  QString value = getPreviousValue(propName);

  const Mantid::Kernel::Property *property = getAlgorithmProperty(propName);

  // Do the right thing for the widget type
  if (QComboBox *opts = qobject_cast<QComboBox *>(widget)) {
    if (property && value.isEmpty()) {
      value = QString::fromStdString(property->value());
    }
    int index = opts->findText(value);
    if (index >= 0) {
      opts->setCurrentIndex(index);
    }
    return;
  }
  if (QAbstractButton *checker = qobject_cast<QAbstractButton *>(widget)) {
    if (value.isEmpty() && dynamic_cast<const Mantid::Kernel::PropertyWithValue<bool> *>(property))
      value = QString::fromStdString(property->value());
    checker->setChecked(value != "0");
    return;
  }
  if (QDateTimeEdit *dateEdit = qobject_cast<QDateTimeEdit *>(widget)) {
    // String in ISO8601 format
    DateAndTime t = DateAndTime::getCurrentTime();
    try {
      t.setFromISO8601(verifyAndSanitizeISO8601(value.toStdString()));
    } catch (std::exception &) {
    }
    dateEdit->setDate(QDate(t.year(), t.month(), t.day()));
    dateEdit->setTime(QTime(t.hour(), t.minute(), t.second(), 0));
    return;
  }

  QLineEdit *textfield = qobject_cast<QLineEdit *>(widget);
  const MantidWidget *mtdwidget = qobject_cast<const MantidWidget *>(widget);
  if (textfield || mtdwidget) {
    if (!isForScript()) {
      if (textfield)
        textfield->setText(value);
      else
        const_cast<MantidWidget *>(mtdwidget)->setUserInput(value);
    } else {
      // Need to check if this is the default value as we don't fill them in if
      // they are
      if (m_python_arguments.contains(propName) || !property->isDefault()) {
        if (textfield)
          textfield->setText(value);
        else
          const_cast<MantidWidget *>(mtdwidget)->setUserInput(value);
      }
    }
    return;
  }

  const PropertyWidget *propWidget = qobject_cast<const PropertyWidget *>(widget);
  if (propWidget) {
    const_cast<PropertyWidget *>(propWidget)->setPreviousValue(value);
    return;
  }

  // Reaching here means we have a widget type we don't understand. Tell the
  // developer
  QMessageBox::warning(this, windowTitle(),
                       QString("Cannot set value for ") + widget->metaObject()->className() +
                           ". Update AlgorithmDialog::setValue() to cope with this widget.");
}

/**
 * Observer the execution of the algorithm using an AlgorithmObserver.
 *
 * All notifications will be observed.
 *
 * @param observer Pointer to the AlgorithmObserver to add.
 */
void AlgorithmDialog::addAlgorithmObserver(Mantid::API::AlgorithmObserver *observer) {
  m_observers.emplace_back(observer);
  // turn off the keep open option - it would only confuse things if someone is
  // watching
  setShowKeepOpen(false);
}

/**Handle completion of algorithm started while staying open.
 * emits another signal to marshal the call to the main ui thread
 *
 * @param alg Completed algorithm (unused)
 */
void AlgorithmDialog::finishHandle(const IAlgorithm *alg) {
  UNUSED_ARG(alg);
  emit algCompletedSignal();
}

/**Handle error signal of algorithm started while staying open.
 * emits another signal to marshal the call to the main ui thread
 *
 * @param alg Completed algorithm (unused)
 * @param what the error message (unused)
 */
void AlgorithmDialog::errorHandle(const IAlgorithm *alg, const std::string &what) {
  UNUSED_ARG(alg);
  UNUSED_ARG(what);
  emit algCompletedSignal();
}

/**
 * Only allow close when close is enabled
 */
void AlgorithmDialog::closeEvent(QCloseEvent *evt) {
  emit closeEventCalled();
  if (m_exitButton) {
    if (m_exitButton->isEnabled()) {
      evt->accept();
    } else {
      evt->ignore();
    }
  } else {
    QDialog::closeEvent(evt);
  }
}

/**Handle completion of algorithm started while staying open.
 * reenables the OK button when the algorithms finishes.
 *
 */
void AlgorithmDialog::algorithmCompleted() {
  if (m_okButton)
    m_okButton->setEnabled(true);
}
