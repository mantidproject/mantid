//----------------------------------
// Includes
//----------------------------------
#include "MantidQtAPI/AlgorithmDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/MantidWidget.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IWorkspaceProperty.h"

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
using namespace MantidQt::API;

//------------------------------------------------------
// Public member functions
//------------------------------------------------------
/**
 * Default Constructor
 */
AlgorithmDialog::AlgorithmDialog(QWidget* parent) :  
  QDialog(parent), m_algorithm(NULL), m_algName(""), m_algProperties(), 
  m_propertyValueMap(), m_tied_properties(), m_forScript(false), m_python_arguments(), 
  m_enabledNames(), m_strMessage(""), m_msgAvailable(false), m_isInitialized(false), 
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

/**
 * Get a property validator label
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
    Mantid::Kernel::Property *p = getAlgorithmProperty(pName);
    QString value = getInputValue(pName);
    QLabel *validator = getValidatorMarker(pName);
    std::string error("");

    try
    {
      if( skipList.contains(pName) )
      {
	error = p->isValid();
      }
      else
      {
	error = p->setValue(value.toStdString());
      }
    }
    catch(std::exception & err_details)
    {
      error = err_details.what();
    }
    if( !error.empty() ) allValid = false;
    // If there's no validator then assume it's handling its own validation notification
    if( validator && validator->parent() )
    {
      validator->setToolTip(QString::fromStdString(error));
      if( error.empty() ) validator->hide();
      else validator->show();
    }
  }
  return allValid;
}

/**
 * Return the message string
 * @returns the message string
 */
const QString & AlgorithmDialog::getOptionalMessage() const
{
  return m_strMessage;
}

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
  inputMessage->setText(getOptionalMessage());
  QHBoxLayout *msgArea = new QHBoxLayout;
  msgArea->addWidget(inputMessage);
  mainLay->addLayout(msgArea);
}

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

/**
 * Check if the control should be enabled for this property
 * @param propName :: The name of the property
 */
bool AlgorithmDialog::isWidgetEnabled(const QString & propName) const
{
  // If this dialog is not for a script then always enable
  if( !isForScript() || propName.isEmpty() )
  {
    return true;
  }

  if( isInEnabledList(propName) ) return true;
  

  // Otherwise it must be disabled but only if it is valid
  Mantid::Kernel::Property *property = getAlgorithmProperty(propName);
  if( property->isValid().empty() && m_python_arguments.contains(propName) )
  {
    return false;
  }
  else
  {
    return true;
  }
}

/**
 * Tie together an input widget and a property
 * @param widget :: The widget that will collect the input
 * @param property :: The name of the property to tie the given widget to
 * @param parent_layout :: An optional pointer to a QLayout class that is reponsible for managing the passed widget.
 * If given, a validator label will be added for the given input widget
 * @param readHistory :: If true then a history value will be retrieved
 * @returns A NULL pointer if a valid label was successfully add to a passed parent_layout otherwise it
 * returns a pointer to the QLabel instance marking the validity
 */
QWidget* AlgorithmDialog::tie(QWidget* widget, const QString & property, QLayout *parent_layout, 
			      bool readHistory)
{
  if( m_tied_properties.contains(property) )
  {
    m_tied_properties.remove(property);
  }
  Mantid::Kernel::Property * prop = getAlgorithmProperty(property);
  if( prop ) 
  { //Set a few things on the widget
    widget->setToolTip(QString::fromStdString(prop->documentation()));
  }
  widget->setEnabled(isWidgetEnabled(property));
  m_tied_properties.insert(property, widget);

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
	int row(0), col(0), span(0);
	grid->getItemPosition(item_index, &row, &col, &span, &span);
	grid->addWidget(validlbl, row, col + 1);
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

  return validlbl;
}

/**
 * Open a file selection box. The type of dialog, i.e. load/save will depend on the
 * property type
 * @param The :: property name that this is associated with. 
 */
QString AlgorithmDialog::openFileDialog(const QString & propName)
{
  if( propName.isEmpty() ) return "";
  Mantid::API::FileProperty* prop = 
    dynamic_cast< Mantid::API::FileProperty* >( getAlgorithmProperty(propName) );
  if( !prop ) return "";

  //The allowed values in this context are file extensions
  std::set<std::string> exts = prop->allowedValues();
  
  /* MG 20/07/09: Static functions such as these that use native Windows and MAC dialogs 
     in those environments are alot faster. This is unforunately at the expense of 
     shell-like pattern matching, i.e. [0-9].      
  */
  QString filename;
  if( prop->isLoadProperty() )
  {
    QString filter;
    if( !exts.empty() )
    {
      // --------- Load a File -------------
      filter = "Files (";
      std::set<std::string>::const_iterator iend = exts.end();
      // Push a wild-card onto the front of each file suffix
      for( std::set<std::string>::const_iterator itr = exts.begin(); itr != iend; ++itr)
      {
        filter.append("*" + QString::fromStdString(*itr) + " ");
      }
      
      filter.trimmed();
      filter.append(QString::fromStdString(")"));
    }
    filter.append(";;All Files (*.*)");

    filename = QFileDialog::getOpenFileName(this, "Open file", AlgorithmInputHistory::Instance().getPreviousDirectory(), filter);
  }
  else if ( prop->isSaveProperty() )
  {
    // --------- Save a File -------------
    //Have each filter on a separate line with the default as the first
    std::string defaultExt = prop->getDefaultExt();
    QString filter; 
    if( !defaultExt.empty() )
    {
      filter = "*" + QString::fromStdString(defaultExt) + ";;";
    }
    std::set<std::string>::const_iterator iend = exts.end();
    for( std::set<std::string>::const_iterator itr = exts.begin(); itr != iend; ++itr)
    {
      if( (*itr) != defaultExt )
      {
        filter.append("*"+QString::fromStdString(*itr) + ";;");
      }
    }
    //Remove last two semi-colons or else we get an extra empty option in the box
    filter.chop(2);
    // Prepend the default filter
    QString selectedFilter;
    filename = QFileDialog::getSaveFileName(this, "Save file", AlgorithmInputHistory::Instance().getPreviousDirectory(), filter, &selectedFilter);
    
    //Check the filename and append the selected filter if necessary
    if( QFileInfo(filename).completeSuffix().isEmpty() )
    {
      // Hack off the first star that the filter returns
      QString ext = selectedFilter;
      if( selectedFilter.startsWith("*") )
      {
	// 1 character from the start
	ext = ext.remove(0,1);
      }
      if( filename.endsWith(".") && ext.startsWith(".") )
      {
	ext = ext.remove(0,1);
      }
      // Construct the full file name
      filename += ext;
    }
  }
  else if ( prop->isDirectoryProperty() )
  {
    filename = QFileDialog::getExistingDirectory(this, "Choose a Directory", AlgorithmInputHistory::Instance().getPreviousDirectory() );
  }
  else
  {
    throw std::runtime_error("Invalid type of file property! This should not happen.");
  }


  if( !filename.isEmpty() ) 
  {
    AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(filename).absoluteDir().path());
  }
  return filename;
}

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

/**
 * Takes the given property and QCheckBox pointer and sets the state based on either
 * the history or property value
 * @param propName :: The name of the property
 * @param 
 * @returns A newed QCheckBox
 */
void AlgorithmDialog::setCheckBoxState(const QString & propName, QCheckBox* checkBox) const
{
  Mantid::Kernel::Property *property = getAlgorithmProperty(propName);
  if( !property ) return;
  
  //Check boxes are special in that if they have a default value we need to display it
  QString displayed("");
  if( !isForScript() )
  {
    displayed = AlgorithmInputHistory::Instance().previousInput(m_algName, propName);
  }
  if( displayed.isEmpty() )
  {
    displayed = QString::fromStdString(property->value());
  }

  if( displayed == "0" )
  {
    checkBox->setCheckState(Qt::Unchecked);
  }
  else
  {
    checkBox->setCheckState(Qt::Checked);
  }

}

/**
 * Set the input for a text box based on either the history or a script value
 * @param propName :: The name of the property
 * @param field :: The QLineEdit field
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
 * Create a button that when clicked will put the name of the input workspace into the
 * output box.
 * @param inputBox :: The input combo box that contains the input workspace names
 * @param outputEdit :: The output text box that should contain the output name
 * @returns A new QPushButton linked to the appropriate widgets.
 */
QPushButton* AlgorithmDialog::createReplaceWSButton(QLineEdit *outputEdit)
{
  QPushButton *btn = new QPushButton(QIcon(":/data_replace.png"), "");
  // MG: There is no way with the QIcon class to actually ask what size it is so I had to hard
  // code this number here to get it to a sensible size
  btn->setMaximumWidth(32);
  m_wsbtn_tracker[btn ] = 1;
  btn->setToolTip("Replace input workspace");
  m_outputws_fields.push_back(outputEdit);
  connect(btn, SIGNAL(clicked()), m_signal_mapper, SLOT(map()));
  m_signal_mapper->setMapping(btn, outputEdit);  
  return btn;
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

/**
 * A slot to handle the help button click
 */
void AlgorithmDialog::helpClicked()
{
  QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") + m_algName));
}

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

/**
  * Set a list of values for the properties
  * @param presetValues :: A string containing a list of "name=value" pairs with each separated by an '|' character
  */
void AlgorithmDialog::setPresetValues(const QString & presetValues)
{
  if( presetValues.isEmpty() ) return;
  QStringList presets = presetValues.split('|', QString::SkipEmptyParts);
  QStringListIterator itr(presets);
  m_python_arguments.clear();
  while( itr.hasNext() )
  {
    QString namevalue = itr.next();
    QString name = namevalue.section('=', 0, 0);
    m_python_arguments.append(name);
    // Simplified removes trims from start and end and replaces all n counts of whitespace with a single whitespace
    QString value = namevalue.section('=', 1, 1).simplified();
    storePropertyValue(name, value.trimmed());
  }
  setPropertyValues();
}

/** 
 * Set comma-separated list of enabled parameter names
 * @param enabledNames :: A comma-separated list of parameter names to keep enabled
 */
void AlgorithmDialog::setEnabledNames(const QString & enabledNames)
{
  if( enabledNames.isEmpty() ) return;
  
  m_enabledNames = enabledNames.split(',', QString::SkipEmptyParts);
}

bool AlgorithmDialog::isInEnabledList(const QString& propName) const
{
  return m_enabledNames.contains(propName);
}

/**
 * Set if we are for a script or not
 * @param forScript :: A boolean inidcating whether we are being called from a script
 */
void AlgorithmDialog::isForScript(bool forScript)
{
  m_forScript = forScript;
}

/**
 * Set an optional message to be displayed at the top of the widget
 * @param message :: The message string
 */
void AlgorithmDialog::setOptionalMessage(const QString & message)
{
  m_strMessage = message;
  if( message.isEmpty() ) m_msgAvailable = false;
}

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
 * @param property :: The property name
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
