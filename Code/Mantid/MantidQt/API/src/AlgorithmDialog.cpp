//----------------------------------
// Includes
//----------------------------------
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidQtAPI/AlgorithmDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/MantidWidget.h"

#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QHBoxLayout>
#include <QSignalMapper>
#include "MantidQtAPI/FilePropertyWidget.h"

using namespace MantidQt::API;
using Mantid::API::IAlgorithm;

//------------------------------------------------------
// Public member functions
//------------------------------------------------------
/**
 * Default Constructor
 */
AlgorithmDialog::AlgorithmDialog(QWidget* parent) :  
  QDialog(parent), m_algorithm(NULL), m_algName(""), m_algProperties(), 
  m_propertyValueMap(), m_tied_properties(), m_forScript(false), m_python_arguments(), 
  m_enabled(), m_disabled(), m_strMessage(""), m_msgAvailable(false), m_isInitialized(false), m_showHidden(true),
  m_validators(), m_noValidation(), m_inputws_opts(), m_outputws_fields(), m_wsbtn_tracker(), 
  m_signal_mapper(new QSignalMapper())
{
  connect(m_signal_mapper, SIGNAL(mapped(QWidget*)), this, SLOT(replaceWSClicked(QWidget*)));
}

/**
 * Destructor
 */
AlgorithmDialog::~AlgorithmDialog()
{
}

void AlgorithmDialog::showHiddenWorkspaces(const bool & show)
{
  m_showHidden = show;
}

/**
 * Create the layout for this dialog.
 */
void AlgorithmDialog::initializeLayout()
{
  if( isInitialized() ) return;

  //Set a common title
  setWindowTitle(QString::fromStdString(getAlgorithm()->name()) + " input dialog");
  //Set the icon
  setWindowIcon(QIcon(":/MantidPlot_Icon_32offset.png"));
  
  // These containers are for ensuring the 'replace input workspace; button works correctly
  // Store all combo boxes that relate to an input workspace
  m_inputws_opts.clear();
  // Store all line edit fields that relate to an output workspace name
  m_outputws_fields.clear();
  // Keep track of the input workspace that has been used to fill the output workspace. Each button click
  // cycles through all of the input workspaces
  m_wsbtn_tracker.clear();

  // This derived class function creates the layout of the widget. It can also add default input if the
  // dialog has been written this way
  this->initLayout();
  // Check if there is any default input 
  this->parse();

  // Try to set these values. This will validate the defaults and mark those that are invalid, if any.
  setPropertyValues();

  m_isInitialized = true;
}

/**
 * Has this dialog been initialized yet
 *  @returns Whether initialzedLayout has been called yet
 */
bool AlgorithmDialog::isInitialized() const
{ 
  return m_isInitialized; 
}


//------------------------------------------------------
// Protected member functions
//------------------------------------------------------
/**
 * Parse input from widgets on the dialog. This function does nothing in the 
 * base class
 */
void AlgorithmDialog::parseInput()
{
}


//-------------------------------------------------------------------------------------------------
/**
 * Save the property values to the input history
 */
void AlgorithmDialog::saveInput()
{
  AlgorithmInputHistory::Instance().clearAlgorithmInput(m_algName);
  QStringList::const_iterator pend = m_algProperties.end();
  for( QStringList::const_iterator pitr = m_algProperties.begin(); pitr != pend; ++pitr )
  {
    Mantid::Kernel::Property* p = getAlgorithmProperty(*pitr);
    if ( p->remember() )
    {
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
void AlgorithmDialog::setAlgorithm(Mantid::API::IAlgorithm* alg)
{
  m_algorithm = alg;
  m_algName = QString::fromStdString(alg->name());
  m_algProperties.clear();
  m_tied_properties.clear();
  std::vector<Mantid::Kernel::Property*>::const_iterator iend = alg->getProperties().end();
  for( std::vector<Mantid::Kernel::Property*>::const_iterator itr = alg->getProperties().begin(); itr != iend;
       ++itr )
  {
    Mantid::Kernel::Property *p = *itr;
    if( dynamic_cast<Mantid::API::IWorkspaceProperty*>(p) || p->direction() != Mantid::Kernel::Direction::Output )
    {
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
Mantid::API::IAlgorithm* AlgorithmDialog::getAlgorithm() const
{
  return m_algorithm;
}

/**
 * Get a named property for this algorithm
 * @param propName :: The name of the property
 */
Mantid::Kernel::Property* AlgorithmDialog::getAlgorithmProperty(const QString & propName) const
{
  if( m_algProperties.contains(propName) ) 
  {
    return m_algorithm->getProperty(propName.toStdString());
  }
  else return NULL;
}

/**
 * Return a true if the given property requires user input
 * @param propName :: The name of the property
 */
bool AlgorithmDialog::requiresUserInput(const QString & propName) const
{
  return m_algProperties.contains(propName);
}


//-------------------------------------------------------------------------------------------------
/** 
 * Get an input value from the form, dealing with blank inputs etc
 * @param propName :: The name of the property
 */
QString AlgorithmDialog::getInputValue(const QString& propName) const
{
  QString value = m_propertyValueMap.value(propName);
  if( value.isEmpty() ) 
  {
    Mantid::Kernel::Property* prop = getAlgorithmProperty(propName);
    if( prop ) return QString::fromStdString(prop->getDefault());
    else return "";
  }
  else return value;
  return value;
}


//-------------------------------------------------------------------------------------------------
/**
 * Get a property validator label (that little red star)
 */
QLabel* AlgorithmDialog::getValidatorMarker(const QString & propname) const
{
  if( m_noValidation.contains(propname) ) return NULL;
  QLabel *validLbl(NULL);
  if( !m_validators.contains(propname) )
  {
    validLbl = new QLabel("*"); 
    QPalette pal = validLbl->palette();
    pal.setColor(QPalette::WindowText, Qt::darkRed);
    validLbl->setPalette(pal);
    m_validators[propname] = validLbl;
  }
  else
  {
    validLbl = m_validators.value(propname);
  }
  return validLbl;
}

/**
 * Adds a property (name,value) pair to the stored map
 */
void AlgorithmDialog::storePropertyValue(const QString & name, const QString & value)
{
  if( name.isEmpty() ) return;
  
  m_propertyValueMap.insert(name, value);
}


//-------------------------------------------------------------------------------------------------
/** Go through all the properties, and check their validators to determine
 * whether they should be made disabled/invisible.
 * It also shows/hids the validators.
 * All properties' values should be set already, otherwise the validators
 * will be running on old data.
 */
void AlgorithmDialog::hideOrDisableProperties()
{
  QStringList::const_iterator pend = m_algProperties.end();
  for( QStringList::const_iterator pitr = m_algProperties.begin(); pitr != pend; ++pitr )
  {
    const QString pName = *pitr;
    Mantid::Kernel::Property *p = getAlgorithmProperty(pName);

    // Find the widget for this property.
    if (m_tied_properties.contains(pName))
    {
      // Set the enabled and visible flags based on what the validators say. Default is always true.
      bool enabled     = isWidgetEnabled(pName);
      bool visible     = p->isVisible();

      if (p->isConditionChanged())
      {
          p->getSettings()->applyChanges(p);
          // TODO: Handle replacing widgets
//          int row = this->deletePropertyWidgets(p);
//          this->createSpecificPropertyWidget(p,row);
      }

      // Show/hide the validator label (that red star)
      QString error = "";
      if (m_errors.contains(pName)) error = m_errors[pName];
      // Always show controls that are in error
      if (error.length() != 0)
        visible = true;

      // Go through all the associated widgets with this property
      QList<QWidget*> widgets = m_tied_all_widgets[pName];
      for (int i=0; i<widgets.size(); i++)
      {

        QWidget * widget = widgets[i];
        widget->setEnabled( enabled );
        widget->setVisible( visible );

      }

      if (visible)
      {
        QLabel *validator = getValidatorMarker(pName);
        // If there's no validator then assume it's handling its own validation notification
        if( validator && validator->parent() )
        {
          validator->setToolTip( error );
          validator->setVisible( error.length() != 0);
        }
      }
    }
  } // for each property

}


//-------------------------------------------------------------------------------------------------
/** Sets the value of a single property, using the value previously stored using
 * storePropertyValue()
 *
 * @param pName :: name of the property to set
 * @param validateOthers :: set to true to validate, enable, or hide ALL other properties after.
 *        Set false if you are setting ALL property values and do it once at the end.
 * @return true if the property is valid.
 */
bool AlgorithmDialog::setPropertyValue(const QString pName, bool validateOthers)
{
  Mantid::Kernel::Property *p = getAlgorithmProperty(pName);
  QString value = getInputValue(pName);

  std::string error("");
  try
  {
    error = p->setValue(value.toStdString());
  }
  catch(std::exception & err_details)
  {
    error = err_details.what();
  }
  // Save the error string for later
  m_errors[pName] = QString::fromStdString(error);

  // Go through all the other properties' validators
  if (validateOthers)
    this->hideOrDisableProperties();

  // Prop was valid if the error string is empty
  return error.empty();
}


//-------------------------------------------------------------------------------------------------
/**
 * Set the properties that have been parsed from the dialog.
 * @param skipList :: An optional list of property names whose values will not be set
 * @returns A boolean that indicates if the validation was successful.
 */
bool AlgorithmDialog::setPropertyValues(const QStringList & skipList)
{
  QStringList::const_iterator pend = m_algProperties.end();
  bool allValid(true);
  for( QStringList::const_iterator pitr = m_algProperties.begin(); pitr != pend; ++pitr )
  {
    const QString pName = *pitr;
    if( skipList.contains(pName) )
    {
      // For the load dialog, skips setting some properties
      Mantid::Kernel::Property *p = getAlgorithmProperty(pName);
      std::string error = p->isValid();
      m_errors[pName] = QString::fromStdString(error);
      if (!error.empty()) allValid = false;
    }
    else
    {
      bool thisValid = this->setPropertyValue(pName, false);
      allValid = allValid && thisValid;
    }
  }

  // OK all the values have been set once. Time to look for which should be enabled
  this->hideOrDisableProperties();

  return allValid;
}


//-------------------------------------------------------------------------------------------------
/**
 * Return the message string
 * @returns the message string
 */
const QString & AlgorithmDialog::getOptionalMessage() const
{
  return m_strMessage;
}


//-------------------------------------------------------------------------------------------------
/** Add the optional message in a light yellow box to the layout
 *
 * @param mainLay :: layout
 */
void AlgorithmDialog::addOptionalMessage(QVBoxLayout *mainLay)
{
  QLabel *inputMessage = new QLabel(this);
  inputMessage->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  QPalette pal = inputMessage->palette();
  pal.setColor(inputMessage->backgroundRole(), QColor(255,255,224)); // Light yellow
  pal.setColor(inputMessage->foregroundRole(), Qt::black);
  inputMessage->setPalette(pal);
  inputMessage->setAutoFillBackground(true);
  inputMessage->setWordWrap(true);
  inputMessage->setAlignment(Qt::AlignJustify);
  inputMessage->setMargin(3);
  inputMessage->setText(getOptionalMessage());
  QHBoxLayout *msgArea = new QHBoxLayout;
  msgArea->addWidget(inputMessage);
  mainLay->addLayout(msgArea);
}


//-------------------------------------------------------------------------------------------------
/**
 * Was this dialog raised from a script? This is important when deciding what to
 * do with properties that have old input
 * @returns A boolean inidcating whether we are being called from a script
 */
bool AlgorithmDialog::isForScript() const
{
  return m_forScript;
}

/*
 * Is there a message string available
 * @returns A boolean indicating whether the message string is empty
 */
bool AlgorithmDialog::isMessageAvailable() const
{
  return !m_strMessage.isEmpty();
}


//-------------------------------------------------------------------------------------------------
/**
 * Check if the control should be enabled for this property
 * @param propName :: The name of the property
 */
bool AlgorithmDialog::isWidgetEnabled(const QString & propName) const
{
  // To avoid errors
  if( propName.isEmpty() ) return true;
  
  // Otherwise it must be disabled but only if it is valid
  Mantid::Kernel::Property *property = getAlgorithmProperty(propName);
  if (!property) return true;


  if( !isForScript() )
  {
    // Regular C++ algo. Let the property tell us,
    // possibly using validators, if it is to be shown enabled
    return property->isEnabled();
  }
  else
  {
    // Algorithm dialog was called from a script(i.e. Python)
    // Keep things enabled if requested
    if( m_enabled.contains(propName) ) return true;

    /**
    * The control is disabled if
    *   (1) It is contained in the disabled list or
    *   (2) A user passed a value into the dialog
    */
    if( m_disabled.contains(propName) || m_python_arguments.contains(propName) )
    {
      return false;
    }
    else
    {
      return true;
    }
  }

}

//-------------------------------------------------------------------------------------------------
/**
 * UnTie a property
 * @param property :: The name of the property to tie the given widget to
 */
void AlgorithmDialog::untie(const QString & property) 
{
  if( m_tied_properties.contains(property) )
  {
    m_tied_properties.remove(property);
  }
}

//-------------------------------------------------------------------------------------------------
/**
 * Tie together an input widget and a property
 * @param widget :: The widget that will collect the input
 * @param property :: The name of the property to tie the given widget to
 * @param parent_layout :: An optional pointer to a QLayout class that is reponsible for managing the passed widget.
 * If given, a validator label will be added for the given input widget
 * @param readHistory :: If true then a history value will be retrieved
 * @param otherWidget1 :: An associated widget that should be hidden if the main one is hidden too.
 * @param otherWidget2 :: An associated widget that should be hidden if the main one is hidden too.
 * @param otherWidget3 :: An associated widget that should be hidden if the main one is hidden too.
 *
 * @return A NULL pointer if a valid label was successfully add to a passed parent_layout otherwise it
 *          returns a pointer to the QLabel instance marking the validity
 */
QWidget* AlgorithmDialog::tie(QWidget* widget, const QString & property, QLayout *parent_layout, 
                  bool readHistory, QWidget * otherWidget1, QWidget * otherWidget2, QWidget * otherWidget3)
{
  if( m_tied_properties.contains(property) )
    m_tied_properties.remove(property);

  if (m_tied_all_widgets.contains(property) )
    m_tied_all_widgets.remove(property);

  Mantid::Kernel::Property * prop = getAlgorithmProperty(property);
  if( prop ) 
  { //Set a few things on the widget
    widget->setToolTip(QString::fromStdString(prop->documentation()));
  }
  widget->setEnabled(isWidgetEnabled(property));

  // Save in the hashes
  m_tied_properties.insert(property, widget);
  QList<QWidget*> allWidgets;
  allWidgets.push_back(widget);
  if (otherWidget1) allWidgets.push_back(otherWidget1);
  if (otherWidget2) allWidgets.push_back(otherWidget2);
  if (otherWidget3) allWidgets.push_back(otherWidget3);

  // If the widget's layout has been given then assume that a validator is required, else assume not
  QWidget* validlbl(NULL);
  if( parent_layout )
  {
    // Check if the validator is already there
    validlbl = getValidatorMarker(property);
    if( validlbl )
    {
      int item_index = parent_layout->indexOf(widget);
      if( QBoxLayout *box = qobject_cast<QBoxLayout*>(parent_layout) )
      {
        box->insertWidget(item_index + 1, validlbl);
      }
      else if( QGridLayout *grid = qobject_cast<QGridLayout*>(parent_layout) )
      {
        // Find the last column that has room for a "INVALID" star
        int row(0), col(0), span(0);
        grid->getItemPosition(item_index, &row, &col, &span, &span);
        int max_col = col;
        for (int i=0; i<allWidgets.size(); i++)
        {
          grid->getItemPosition( parent_layout->indexOf(allWidgets[i]), &row, &col, &span, &span);
          if (col > max_col) max_col = col;
        }

        grid->addWidget(validlbl, row, max_col + 1);
      }
      else {}
    }
  }
  else
  {
    m_noValidation.append(property);
  }

  if( readHistory )
  {
    setPreviousValue(widget, property);
  }

  // Save all the associated widgets
  if (validlbl) allWidgets.push_back(validlbl);
  m_tied_all_widgets.insert(property, allWidgets);

  return validlbl;
}



//-------------------------------------------------------------------------------------------------
/**
 * Open a file selection box. The type of dialog, i.e. load/save will depend on the
 * property type
 * @param propName :: The property name that this is associated with.
 */
QString AlgorithmDialog::openFileDialog(const QString & propName)
{
  if( propName.isEmpty() ) return "";
  return FilePropertyWidget::openFileDialog( this->getAlgorithmProperty(propName) );
}




//-------------------------------------------------------------------------------------------------
/**
 * Takes a combobox and adds the allowed values of the given property to its list. 
 * It also sets the displayed value to the correct one based on either the history
 * or a script input value
 * @param propName :: The name of the property
 * @param optionsBox :: A pointer to a QComoboBox object
 * @returns A newed QComboBox
 */
void AlgorithmDialog::fillAndSetComboBox(const QString & propName, QComboBox* optionsBox) const
{
  if( !optionsBox ) return;
  Mantid::Kernel::Property *property = getAlgorithmProperty(propName);
  if( !property ) return;
  
  std::set<std::string> items = property->allowedValues();
  std::set<std::string>::const_iterator vend = items.end();
  for(std::set<std::string>::const_iterator vitr = items.begin(); vitr != vend; 
      ++vitr)
  {
    optionsBox->addItem(QString::fromStdString(*vitr));
  }

  // Display the appropriate value
  QString displayed("");
  if( !isForScript() )
  {
    displayed = AlgorithmInputHistory::Instance().previousInput(m_algName, propName);
  }
  if( displayed.isEmpty() )
  {
    displayed = QString::fromStdString(property->value());
  }

  int index = optionsBox->findText(displayed);
  if( index >= 0 )
  {
    optionsBox->setCurrentIndex(index);
  }
}


//-------------------------------------------------------------------------------------------------
/**
 * Set the input for a text box based on either the history or a script value
 * @param propName :: The name of the property
 * @param textField :: The QLineEdit field
 */
void AlgorithmDialog::fillLineEdit(const QString & propName, QLineEdit* textField)
{
  if( !isForScript() )
  {
    textField->setText(AlgorithmInputHistory::Instance().previousInput(m_algName, propName));
  }
  else
  {
    Mantid::Kernel::Property *property = getAlgorithmProperty(propName);
    if( property && property->isValid().empty() && 
    ( m_python_arguments.contains(propName) || !property->isDefault() ) ) 
    {
      textField->setText(QString::fromStdString(property->value()));
    }
  }
}



//-------------------------------------------------------------------------------------------------
/** Layout the buttons and others in the generic dialog */
QHBoxLayout *
AlgorithmDialog::createDefaultButtonLayout(const QString & helpText,
                       const QString & loadText,
                       const QString & cancelText)
{
  QPushButton *okButton = new QPushButton(loadText);
  connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
  okButton->setDefault(true);

  QPushButton *exitButton = new QPushButton(cancelText);
  connect(exitButton, SIGNAL(clicked()), this, SLOT(close()));

  QHBoxLayout *buttonRowLayout = new QHBoxLayout;
  buttonRowLayout->addWidget(createHelpButton(helpText));
  buttonRowLayout->addStretch();
  buttonRowLayout->addWidget(okButton);
  buttonRowLayout->addWidget(exitButton);
    
  return buttonRowLayout;
}


//-------------------------------------------------------------------------------------------------
/**
 * Create a help button that, when clicked, will open a browser to the Mantid wiki page
 * for that algorithm
 */
QPushButton* AlgorithmDialog::createHelpButton(const QString & helpText) const
{
  QPushButton *help = new QPushButton(helpText);
  help->setMaximumWidth(25);
  connect(help, SIGNAL(clicked()), this, SLOT(helpClicked()));
  return help;
}



/** 
 * Flag an input workspace widget
 * @param inputWidget :: A widget used to enter the input workspace
 */
void AlgorithmDialog::flagInputWS(QWidget *inputWidget)
{
  m_inputws_opts.push_back(inputWidget);
}

//-----------------------------------------------------------
// Protected slots
//-----------------------------------------------------------
/**
 * A slot that can be used to connect a button that accepts the dialog if
 * all of the properties are valid
 */
void AlgorithmDialog::accept()
{
  // Get property values
  parse();
  
  //Try and set and validate the properties and 
  if( setPropertyValues() )
  {
    //Store input for next time
    saveInput();
    QDialog::accept();
  }
  else
  {
    QMessageBox::critical(this, "", 
              "One or more properties are invalid. The invalid properties are\n"
        "marked with a *, hold your mouse over the * for more information." );
  } 
}


//-------------------------------------------------------------------------------------------------
/**
 * A slot to handle the help button click
 */
void AlgorithmDialog::helpClicked()
{
  // Default help URL
  QString url = QString("http://www.mantidproject.org/") + m_algName;

  if (m_algorithm)
  {
    // Find the latest version
    IAlgorithm* alg = Mantid::API::FrameworkManager::Instance().createAlgorithm(m_algName.toStdString(), -1);
    int latest_version = alg->version();
    // Adjust the link if you're NOT looking at the latest version of the algo
    int this_version = m_algorithm->version();
    if ((this_version != latest_version))
      url += "_v." + QString::number(this_version);
  }

  // Open the URL
  QDesktopServices::openUrl(QUrl(url));
}

//-------------------------------------------------------------------------------------------------
/**
 * A slot to handle the replace workspace button click
 * @param outputEdit :: The line edit that is associated, via the signalmapper, with this click
 */
void AlgorithmDialog::replaceWSClicked(QWidget *outputEdit)
{
  QPushButton *btn = qobject_cast<QPushButton*>(m_signal_mapper->mapping(outputEdit));
  if( !btn ) return;
  int input =  m_wsbtn_tracker.value(btn);

  QWidget *wsInputWidget = m_inputws_opts.value(input-1);
  QString wsname(""); 
  if( QComboBox *options = qobject_cast<QComboBox*>(wsInputWidget) )
  {
    wsname = options->currentText();
  }
  else if( QLineEdit *editField = qobject_cast<QLineEdit*>(wsInputWidget) )
  {
    wsname = editField->text();
  }
  else return;

  //Adjust tracker
  input = (input % m_inputws_opts.size() ) + 1;
  m_wsbtn_tracker[btn] = input;

  // Check if any of the other line edits have this name
  QVector<QLineEdit*>::const_iterator iend = m_outputws_fields.constEnd();
  for( QVector<QLineEdit*>::const_iterator itr = m_outputws_fields.constBegin();
       itr != iend; ++itr )
  {
    //Check that we are not the field we are actually comparing against
    if( (*itr) == outputEdit ) continue;
    if( (*itr)->text() == wsname )
    {
      wsname += "-1";
      break;
    }
  }
  QLineEdit *edit = qobject_cast<QLineEdit*>(outputEdit);
  if( edit )
  {
    edit->setText(wsname);
  }
}

//------------------------------------------------------
// Private member functions
//------------------------------------------------------
/**
 * Parse out information from the dialog
 */
void AlgorithmDialog::parse()
{
  QHashIterator<QString, QWidget*> itr(m_tied_properties);
  while( itr.hasNext() )
  {
    itr.next();
    //Need to do different things depending on the type of the widget. getValue sorts this out
    storePropertyValue(itr.key(), getValue(itr.value()));
  }

  //Now call parseInput, which can be overridden in an inheriting class
  parseInput();
}

//-------------------------------------------------------------------------------------------------
/**
  * Set a list of values for the properties
  * @param presetValues :: A string containing a list of "name=value" pairs with each separated by an '|' character
  */
void AlgorithmDialog::setPresetValues(const QHash<QString,QString> & presetValues)
{
  if( presetValues.isEmpty() ) return;
  QHashIterator<QString,QString> itr(presetValues);
  m_python_arguments.clear();
  while( itr.hasNext() )
  {
    itr.next();
    QString name = itr.key();
    m_python_arguments.append(name);
    QString value = itr.value();
    storePropertyValue(name, value);
  }
  setPropertyValues();
}

/** 
 * Set list of enabled and disabled parameter names
 * @param enabled:: A list of parameter names to keep enabled
 * @param disabled:: A list of parameter names whose widgets should be disabled
 */
void AlgorithmDialog::addEnabledAndDisableLists(const QStringList & enabled, const QStringList & disabled)
{
  m_enabled = enabled;
  m_disabled = disabled;
}

/**
 * Returns true if the parameter name has been explicity requested to be kept enabled. If the parameter
 * has been explicity requested to be disabled then return false as well as if neither have been specified
 */
bool AlgorithmDialog::requestedToKeepEnabled(const QString& propName) const
{
  bool enabled(true);
  if( m_disabled.contains(propName) ) 
  {
    enabled = false;
  }
  else if( m_enabled.contains(propName) ) // Definitely enable
  {
    enabled = true;
  }
  else //Nothing was specified
  {
    enabled = false;
  }
  return enabled;
}

/**
 * Set if we are for a script or not
 * @param forScript :: A boolean inidcating whether we are being called from a script
 */
void AlgorithmDialog::isForScript(bool forScript)
{
  m_forScript = forScript;
}

//-------------------------------------------------------------------------------------------------
/**
 * Set an optional message to be displayed at the top of the widget
 * @param message :: The message string
 */
void AlgorithmDialog::setOptionalMessage(const QString & message)
{
  m_strMessage = message;
  if( message.isEmpty() ) m_strMessage = QString::fromStdString(getAlgorithm()->getOptionalMessage());
  if( m_strMessage.isEmpty() ) m_msgAvailable = false;
  else m_msgAvailable = true;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get a value from a widget. The function needs to know about the types of widgets
 * that are being used. Currently it knows about QComboBox, QLineEdit, QCheckBox and MWRunFiles
 * @param widget :: A pointer to the widget
 */
QString AlgorithmDialog::getValue(QWidget *widget)
{
  if( QComboBox *opts = qobject_cast<QComboBox*>(widget) )
  {
    return opts->currentText().trimmed();
  }
  else if( QLineEdit *textfield = qobject_cast<QLineEdit*>(widget) )
  {
    return textfield->text().trimmed();
  }
  else if( QCheckBox *checker = qobject_cast<QCheckBox*>(widget) )
  {
    if( checker->checkState() == Qt::Checked )
    {
      return QString("1");
    }
    else
    {
      return QString("0");
    }
  }
  else if( MantidWidget *mtd_widget = qobject_cast<MantidWidget*>(widget) )
  {
    return mtd_widget->getUserInput().toString().trimmed();
  }
  else
  {
    QMessageBox::warning(this, windowTitle(), 
             QString("Cannot parse input from ") + widget->metaObject()->className() + 
             ". Update AlgorithmDialog::getValue() to cope with this widget.");
    return "";
  }
}

/**
 * Set a value for a widget. The function needs to know about the types of widgets
 * that are being used. Currently it knows about QComboBox, QLineEdit and QCheckBox
 * @param widget :: A pointer to the widget
 * @param propName :: The property name
 */
void AlgorithmDialog::setPreviousValue(QWidget *widget, const QString & propName)
{
  // Get the value from either the previous input store or from Python argument
  QString value("");
  Mantid::Kernel::Property *property = getAlgorithmProperty(propName);

  if( !isForScript() )
  {
    value = m_propertyValueMap.value(propName);
    if( value.isEmpty() )
    {
      value = AlgorithmInputHistory::Instance().previousInput(m_algName, propName);
    }
  }
  else
  {
    if( !property ) return;
    value = m_propertyValueMap.value(propName);
  }

  // Do the right thing for the widget type
  if( QComboBox *opts = qobject_cast<QComboBox*>(widget) )
  {
    if( property && value.isEmpty() )
    {
      value = QString::fromStdString(property->value());
    }
    int index = opts->findText(value);
    if( index >= 0 )
    {
      opts->setCurrentIndex(index);
    }
    return;
  }
  if( QCheckBox *checker = qobject_cast<QCheckBox*>(widget) )
  {
    if( value.isEmpty() && dynamic_cast<Mantid::Kernel::PropertyWithValue<bool>* >(property) )
    {
      value = QString::fromStdString(property->value());
    }
    if( value == "0" )
    {
      checker->setCheckState(Qt::Unchecked);
    }
    else
    {
      checker->setCheckState(Qt::Checked);
    }
    return;
  }

  QLineEdit *textfield = qobject_cast<QLineEdit*>(widget);
  MantidWidget *mtdwidget = qobject_cast<MantidWidget*>(widget);
  if( textfield || mtdwidget )	    
  {
    if( !isForScript() )
    {
      if( textfield ) textfield->setText(value);
      else mtdwidget->setUserInput(value);
    }
    else
    {
      //Need to check if this is the default value as we don't fill them in if they are
      if( m_python_arguments.contains(propName) || !property->isDefault() )
      {
    if( textfield ) textfield->setText(value);
    else mtdwidget->setUserInput(value);
      }
    }
    return;
  }
  
  // Reaching here means we have a widget type we don't understand. Tell the developer
  QMessageBox::warning(this, windowTitle(), 
               QString("Cannot set value for ") + widget->metaObject()->className() + 
               ". Update AlgorithmDialog::setValue() to cope with this widget.");
}
