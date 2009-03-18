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

using namespace MantidQt::API;

//------------------------------------------------------
// Public member functions
//------------------------------------------------------
/**
 * Default Constructor
 */
AlgorithmDialog::AlgorithmDialog(QWidget* parent) :  
  QDialog(parent), m_algorithm(NULL), m_algName(""), m_propertyValueMap(), m_forScript(false), m_strMessage(""), 
  m_msgAvailable(false), m_bIsInitialized(false), m_algProperties(),  m_validators()
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
  
  //Calls the derived class function
  this->initLayout();
   
  //Check which properties are already valid. This also hides the validation marker
  //if a property is valid.
  validateProperties();

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
 * Are we for a script or not
 * @returns A boolean inidcating whether we are being called from a script
 */
bool AlgorithmDialog::isForScript() const
{
  return m_forScript;
}

/**
 * Return the message string
 * @returns the message string
 */
const QString & AlgorithmDialog::getOptionalMessage() const
{
  return m_strMessage;
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
  return m_validators[propname];
}

/**
 * Adds a property (name,value) pair to the stored map
 */
void AlgorithmDialog::addPropertyValueToMap(const QString & name, const QString & value)
{
  if( name.isEmpty() || value.isEmpty() ) return;
  
  m_propertyValueMap.insert(name, value);
}

bool AlgorithmDialog::validateProperties()
{
  bool allValid(true);
  QMap<QString, Mantid::Kernel::Property*>::const_iterator pend = m_algProperties.end();
  for( QMap<QString, Mantid::Kernel::Property*>::const_iterator pitr = m_algProperties.begin();
       pitr != pend; ++pitr )
  {
    const Mantid::Kernel::Property *prop = pitr.value();
    QLabel *validator = getValidatorMarker(pitr.key());
    // Here we have a property that was able to be set
    if( prop->isValid() ) 
    {
      validator->hide();
    }
    else
    {
      allValid = false;
      if( validator->parent() ) validator->show();
    }
  }
  return allValid;
}

/**
 * Set the properties that have been parsed from the dialog.
 * @returns A boolean that indicates if the validation was successful.
 */
bool AlgorithmDialog::setPropertyValues()
{
  QMap<QString, Mantid::Kernel::Property*>::const_iterator pend = m_algProperties.end();
  QString algName = QString::fromStdString(getAlgorithm()->name());
  AlgorithmInputHistory::Instance().clearAlgorithmInput(algName);
  bool allValid(true);
  for( QMap<QString, Mantid::Kernel::Property*>::const_iterator pitr = m_algProperties.begin();
       pitr != pend; ++pitr )
  {
    Mantid::Kernel::Property *prop = pitr.value();
    QString pName = pitr.key();
    QString value = m_propertyValueMap.value(pName);
    QLabel *validator = getValidatorMarker(pitr.key());
    if( !value.isEmpty() )
    {
      prop->setValue(value.toStdString());
    }
    if( prop->isValid() )
    {
      validator->hide();
      //Store value for future input
      AlgorithmInputHistory::Instance().storeNewValue(algName, QPair<QString, QString>(pName, value));
    }
    else
    {
      allValid = false;
      if( validator->parent() ) validator->show();
    }
  }
  //  return validateProperties();
  return allValid;
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
  
  QString filepath; 
  const Mantid::Kernel::FileValidator *file_checker = dynamic_cast<const Mantid::Kernel::FileValidator*>(prop->getValidator());
  if( file_checker && !file_checker->fileMustExist() )
  {
    filepath = QFileDialog::getSaveFileName(this, tr("Select File"), AlgorithmInputHistory::Instance().getPreviousDirectory(), filter);
  }
  else
  {
    filepath = QFileDialog::getOpenFileName(this, tr("Select File"), AlgorithmInputHistory::Instance().getPreviousDirectory(), filter);
  }
  
  if( !filepath.isEmpty() ) AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(filepath).absoluteDir().path());
  return filepath;
}

/**
 * Set old input for text edit field
 * @param propName The name of the property
 * @param field The QLineEdit field
 */
void AlgorithmDialog::setOldLineEditInput(const QString & propName, QLineEdit* field)
{
  Mantid::Kernel::Property *prop = getAlgorithmProperty(propName);
  if( !prop ) return;
  if( isForScript() && prop->isValid() && !prop->isDefault() )
  {
    field->setText(QString::fromStdString(prop->value()));
    field->setEnabled(false);
  }
  else
  {
    field->setText(AlgorithmInputHistory::Instance().previousInput(m_algName, propName));
  }
}


/**
 * A slot that can be used to connect a button that accepts the dialog if
 * all of the properties are valid
 */
void AlgorithmDialog::accept()
{
  parseInput();
  
  if( setPropertyValues() ) QDialog::accept();
  else
  {
    QMessageBox::critical(this, "", 
			  "One or more properties are invalid. They are marked with a *");
  } 
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
  QMap<QString, Mantid::Kernel::Property*>::const_iterator pend = m_algProperties.end();
  for( QMap<QString, Mantid::Kernel::Property*>::const_iterator pitr = m_algProperties.begin();
       pitr != pend; ++pitr )
  {
    QLabel *validLbl = new QLabel("*");
    QPalette pal = validLbl->palette();
    pal.setColor(QPalette::WindowText, Qt::darkRed);
    validLbl->setPalette(pal);
    m_validators[pitr.key()] = validLbl;
  }
}
