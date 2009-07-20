//----------------------------------
// Includes
//----------------------------------
#include "MantidQtAPI/AlgorithmDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/FileValidator.h"

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

using namespace MantidQt::API;

//------------------------------------------------------
// Public member functions
//------------------------------------------------------
/**
 * Default Constructor
 */
AlgorithmDialog::AlgorithmDialog(QWidget* parent) :  
  QDialog(parent), m_algorithm(NULL), m_algName(""), m_propertyValueMap(), m_enabledNames(),
  m_forScript(false), m_python_arguments(), m_strMessage(""), m_msgAvailable(false), 
  m_bIsInitialized(false), m_algProperties(), m_validators()
{
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

  createValidatorLabels();
  
  // This derived class function creates the layout of the widget. It can also add default input if the
  // dialog has been written this way
  this->initLayout();
  // Check if there is any default input 
  this->parseInput();
  // Try to set these values. This will validate the defaults and mark those that are invalid, if any.
  setPropertyValues();

  m_bIsInitialized = true;
}

/**
 * Has this dialog been initialized yet
 *  @returns Whether initialzedLayout has been called yet
 */
bool AlgorithmDialog::isInitialized() const
{ 
  return m_bIsInitialized; 
}


//------------------------------------------------------
// Protected member functions
//------------------------------------------------------
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
 * @param propName The name of the property
 */
Mantid::Kernel::Property* AlgorithmDialog::getAlgorithmProperty(const QString & propName) const
{
  if( m_algProperties.contains(propName) ) return m_algProperties[propName];
  else return NULL;
}

/**
 * Get a property validator label
 */
QLabel* AlgorithmDialog::getValidatorMarker(const QString & propname) const
{
  return m_validators.value(propname);
}

/**
 * Return the message string
 * @returns the message string
 */
const QString & AlgorithmDialog::getOptionalMessage() const
{
  return m_strMessage;
}

/**
 * Are we for a script or not
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
 * @param propName The name of the property
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
  if( property->isValid().empty() )
  {
    return false;
  }
  else
  {
    return true;
  }
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
 * Open a file selection box
 * @param The property name that this is associated with
 */
QString AlgorithmDialog::openLoadFileDialog(const QString & propName)
{
  if( propName.isEmpty() ) return "";
  Mantid::Kernel::PropertyWithValue<std::string>* prop = 
    dynamic_cast< Mantid::Kernel::PropertyWithValue<std::string>* >( getAlgorithmProperty(propName) );
  if( !prop ) return "";

  //The allowed values in this context are file extensions
  std::vector<std::string> exts = prop->allowedValues();
  QString filter;
  if( !exts.empty() )
  {
    filter = "Files (";
		
    std::vector<std::string>::const_iterator iend = exts.end();
    for( std::vector<std::string>::const_iterator itr = exts.begin(); itr != iend; ++itr)
    {
  	  filter.append("*." + QString::fromStdString(*itr) + " ");
    }
		
    filter.trimmed();
    filter.append(QString::fromStdString(")"));
  }
  else
  {
    filter = "All Files (*.*)";
  }
  
  const Mantid::Kernel::FileValidator *file_checker = dynamic_cast<const Mantid::Kernel::FileValidator*>(prop->getValidator());
  
  /* MG 20/07/09: Static functions as these then use native Windows and MAC dialogs in those environments and are alot faster */
  
  //QFileDialog dialog(this);
  //dialog.setNameFilter(filter);
  //dialog.setDirectory(AlgorithmInputHistory::Instance().getPreviousDirectory());
  //if( file_checker && !file_checker->fileMustExist() )
  //{
  //  dialog.setFileMode(QFileDialog::AnyFile);
  //}
  //else
  //{
  //  dialog.setFileMode(QFileDialog::ExistingFile);
  //}
  QString filename;
  if( file_checker && !file_checker->fileMustExist() )
  {
    filename = QFileDialog::getSaveFileName(this, "Save file", AlgorithmInputHistory::Instance().getPreviousDirectory(), filter);
  }
  else
  {
    filename = QFileDialog::getOpenFileName(this, "Open file", AlgorithmInputHistory::Instance().getPreviousDirectory(), filter);
  }

  //QStringList file_names;
  //if (dialog.exec()) file_names = dialog.selectedFiles();
  
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
 * @param propName The name of the property
 * @param optionsBox A pointer to a QComoboBox object
 * @returns A newed QComboBox
 */
void AlgorithmDialog::fillAndSetComboBox(const QString & propName, QComboBox* optionsBox) const
{
  if( !optionsBox ) return;
  Mantid::Kernel::Property *property = getAlgorithmProperty(propName);
  if( !property ) return;
  
  std::vector<std::string> items = property->allowedValues();
  std::vector<std::string>::const_iterator vend = items.end();
  for(std::vector<std::string>::const_iterator vitr = items.begin(); vitr != vend; 
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
 * @param propName The name of the property
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
 * @param propName The name of the property
 * @param field The QLineEdit field
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
 * A slot that can be used to connect a button that accepts the dialog if
 * all of the properties are valid
 */
void AlgorithmDialog::accept()
{
  parseInput();
  
  if( setPropertyValues() )
  {
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

//------------------------------------------------------
// Private member functions
//------------------------------------------------------
/**
 * Set the algorithm pointer
 * @param alg A pointer to the algorithm
 */
void AlgorithmDialog::setAlgorithm(Mantid::API::IAlgorithm* alg)
{
  m_algorithm = alg;
  m_algName = QString::fromStdString(alg->name());
  m_algProperties.clear();
  std::vector<Mantid::Kernel::Property*>::const_iterator iend = alg->getProperties().end();
  for( std::vector<Mantid::Kernel::Property*>::const_iterator itr = alg->getProperties().begin(); itr != iend;
       ++itr )
  {
    m_algProperties.insert(QString::fromStdString((*itr)->name()), *itr); 
  }
}

/**
 * Set the properties that have been parsed from the dialog.
 * @returns A boolean that indicates if the validation was successful.
 */
bool AlgorithmDialog::setPropertyValues()
{
  QHash<QString, Mantid::Kernel::Property*>::const_iterator pend = m_algProperties.end();
  bool allValid(true);
  for( QHash<QString, Mantid::Kernel::Property*>::const_iterator pitr = m_algProperties.begin();
       pitr != pend; ++pitr )
  {
    Mantid::Kernel::Property *prop = pitr.value();
    QString pName = pitr.key();
    QString value = m_propertyValueMap.value(pName);
    QLabel *validator = getValidatorMarker(pitr.key());

    std::string error = "";
    if ( !value.isEmpty() )
    {//if there something in the box then use it
      error = prop->setValue(value.toStdString());
    }
    else
    {//else use the default with may or may not be a valid property value
      error = prop->setValue(prop->getDefault());
    }

    if( error.empty() )
    {//no error
      if( validator ) validator->hide();
      //Store value for future input if it is not default
    }
    else
    {//the property could not be set
      allValid = false;
      if( validator && validator->parent() )
      {
        //a description of the problem will be visible to users if they linger their mouse over validator star mark
        validator->setToolTip(  QString::fromStdString(error) );
        validator->show();
      }
    }
  }
  return allValid;
}

/**
 * Save the property values to the input history
 */
void AlgorithmDialog::saveInput()
{
  AlgorithmInputHistory::Instance().clearAlgorithmInput(m_algName);
  QHash<QString, Mantid::Kernel::Property*>::const_iterator pend = m_algProperties.end();
  for( QHash<QString, Mantid::Kernel::Property*>::const_iterator pitr = m_algProperties.begin();
       pitr != pend; ++pitr )
  {
    QString pName = pitr.key();
    QString value = m_propertyValueMap.value(pName);
    AlgorithmInputHistory::Instance().storeNewValue(m_algName, QPair<QString, QString>(pName, value));
  }
}

/**
  * Set a list of values for the properties
  * @param presetValues A string containing a list of "name=value" pairs with each separated by an '|' character
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
  m_propertyValueMap.clear();
}

/** 
 * Set comma-separated list of enabled parameter names
 * @param enabledNames A comma-separated list of parameter names to keep enabled
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
 * @param forScript A boolean inidcating whether we are being called from a script
 */
void AlgorithmDialog::isForScript(bool forScript)
{
  m_forScript = forScript;
}

/**
 * Set an optional message to be displayed at the top of the widget
 * @param message The message string
 */
void AlgorithmDialog::setOptionalMessage(const QString & message)
{
  m_strMessage = message;
  if( message.isEmpty() ) m_msgAvailable = false;
}

/**
 * This sets up the labels that are to be used to mark whether a property is valid. It has
 * a default implmentation but can be overridden if some other marker is required
 */ 
void AlgorithmDialog::createValidatorLabels()
{
  QHash<QString, Mantid::Kernel::Property*>::const_iterator pend = m_algProperties.end();
  for( QHash<QString, Mantid::Kernel::Property*>::const_iterator pitr = m_algProperties.begin();
       pitr != pend; ++pitr )
  {
    QLabel *validLbl = new QLabel("*");
    QPalette pal = validLbl->palette();
    pal.setColor(QPalette::WindowText, Qt::darkRed);
    validLbl->setPalette(pal);
    m_validators[pitr.key()] = validLbl;
  }
}
