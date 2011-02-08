#include "MantidQtMantidWidgets/MWRunFiles.h"

#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/AlgorithmManager.h"

#include <QStringList>
#include <QFileDialog>
#include <QFileInfo>

#include "Poco/File.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

static const int MAX_FILE_NOT_FOUND_DISP = 3;

MWRunFiles::MWRunFiles(QWidget *parent) : MantidWidget(parent),
  m_findRunFiles(true), m_allowMultipleFiles(true), m_isOptional(false),
  m_doMultiEntry(false), m_fileProblem(""), m_entryNumProblem(""),
  m_algorithmProperty(""), m_fileFilter("")
{
  m_uiForm.setupUi(this);

  connect(m_uiForm.fileEditor, SIGNAL(textChanged(const QString &)), this, SIGNAL(fileTextChanged(const QString&)));
  connect(m_uiForm.browseBtn, SIGNAL(clicked()), this, SLOT(browseClicked()));

  connect(m_uiForm.fileEditor, SIGNAL(editingFinished()), this, SIGNAL(fileEditingFinished()));
  connect(this, SIGNAL(fileEditingFinished()), this, SLOT(findFiles()));
  connect(m_uiForm.entryNum, SIGNAL(textChanged(const QString &)), this, SLOT(checkEntry()));
  connect(m_uiForm.entryNum, SIGNAL(editingFinished()), this, SLOT(checkEntry()));

  m_uiForm.fileEditor->clear();
  
  m_doMultiEntry ? m_uiForm.entryNum->show() : m_uiForm.entryNum->hide();

  setFocusPolicy(Qt::StrongFocus);
  setFocusProxy(m_uiForm.fileEditor);

  findFiles();
}

/**
* Returns if this widget is for run file searching or not
* @returns True if this widget searches for run files, false otherwise
*/
bool MWRunFiles::isForRunFiles() const
{
  return m_findRunFiles;
}

/**
* Sets whether this widget is for run file searching or not
* @param True :: if this widget searches for run files, false otherwise
*/
void MWRunFiles::isForRunFiles(const bool mode)
{
  m_findRunFiles = mode;
}

/**
* Return the label text on the widget
* @returns The current value of the text on the label
*/
QString MWRunFiles::getLabelText() const
{ 
  return m_uiForm.textLabel->text();
}

/**
* Set the text on the label
* @param text :: A string giving the label to use for the text
*/
void MWRunFiles::setLabelText(const QString & text) 
{ 
  m_uiForm.textLabel->setText(text);
}

/**
* Return whether this widget allows multiple files to be specified within the edit box
* @returns True if multiple files can be specified, false otherwise
*/
bool MWRunFiles::allowMultipleFiles() const
{
  return m_allowMultipleFiles;
}

/**
* Set whether this widget allows multiple files to be specifed or not
* @param allow :: If true then the widget will accept multiple files else only a single file may be specified
*/
void MWRunFiles::allowMultipleFiles(const bool allow)
{
  m_allowMultipleFiles = allow;
  findFiles();
}

/**
* Return whether empty input is allowed
*/
bool MWRunFiles::isOptional() const
{
  return m_isOptional;
}

/**
* Sets if the text field is optional
* @param optional :: Set the optional status of the text field
*/
void MWRunFiles::isOptional(const bool optional)
{
  m_isOptional = optional;
  findFiles();
}

/**
* Whether to find the number of entries in the file or assume (the
* normal situation) of one entry
* @return true if the widget is to look for multiple entries
*/
bool MWRunFiles::doMultiEntry() const
{
  return m_doMultiEntry;
}

/**
* Set to true to enable the period number box
* @param multiEntry whether to show the multiperiod box
*/
void MWRunFiles::doMultiEntry(const bool multiEntry)
{
  m_doMultiEntry = multiEntry;
  if (m_doMultiEntry)
  {
    m_uiForm.entryNum->show();
  }
  else
  {
    m_uiForm.entryNum->hide();
  }
  refreshValidator();
}

/**
* Returns the algorithm name
* @returns The algorithm name
*/
QString MWRunFiles::getAlgorithmProperty() const
{
  return m_algorithmProperty;
}

/**
* Sets an algorithm name that can be tied to this widget
* @param name :: The name of the algorithm and property in the form [AlgorithmName|PropertyName]
*/
void MWRunFiles::setAlgorithmProperty(const QString & text)
{
  m_algorithmProperty = text;
}
/**
* Returns the list of file extensions the widget will search for.
* @return list of file extensions
*/
QStringList MWRunFiles::getFileExtensions() const
{
  return m_fileExtensions;
}
/**
* Sets the list of file extensions the dialog will search for. Only taken notice of if AlgorithmProperty not set.
* @param extensions :: list of file extensions
*/
void MWRunFiles::setFileExtensions(const QStringList & extensions)
{
  m_fileExtensions = extensions;
  m_fileFilter.clear();
}

/**
* Is the input within the widget valid?
* @returns True of the file names within the widget are valid, false otherwise
*/
bool MWRunFiles::isValid() const
{
  if( m_uiForm.valid->isHidden() )
  {
    return true;
  }
  else
  {
    return false;
  }
}

/** 
* Returns the names of the files found
* @return an array of filenames entered in the box
*/
QStringList MWRunFiles::getFilenames() const
{
  return m_foundFiles;
}

/** Safer than using getRunFiles()[0] in the situation were there are no files
*  @return an empty string is returned if no input files have been defined or one of the files can't be found, otherwise it's the name of the first input file
*  @throw invalid_argument if one of the files couldn't be found it then no filenames are returned
*/
QString MWRunFiles::getFirstFilename() const
{
  if( m_foundFiles.isEmpty() )
    return "";
  else
    return m_foundFiles[0];
}

/** The number the user entered into the entryNum lineEdit
* or NO_ENTRY_NUM on error. Checking if isValid is true should
* eliminate the possiblity of getting NO_ENTRY_NUM
*/
int MWRunFiles::getEntryNum() const
{
  if ( m_uiForm.entryNum->text().isEmpty() || (! m_doMultiEntry) )
  {
    return ALL_ENTRIES;
  }
  if  (isValid())
  {
    bool isANumber;
    const int period = m_uiForm.entryNum->text().toInt(&isANumber);
    if (isANumber)
    {
      return period;
    }
  }
  return NO_ENTRY_NUM;
}

/**
 * Retrieve user input from this widget. NOTE: This knows nothing of periods yet
 * @returns A qvariant
 */
QVariant MWRunFiles::getUserInput() const
{
  return QVariant(m_uiForm.fileEditor->text());
}

/**
 * Sets a value on the widget through a common interface. The 
 * QVariant is assumed to be text and to contain a file string. Note that this
 * is primarily here for use within the AlgorithmDialog
 * @param value A QVariant containing user text
 */
void MWRunFiles::setUserInput(const QVariant & value)
{
  m_uiForm.fileEditor->setText(value.toString());
  emit fileEditingFinished();
}

/**
 * Flag a problem with the file the user entered, an empty string means no error but
 * there may be an error with the entry box if enabled. Errors passed here are shown first
 * @param A message to include or "" for no error
 */
void MWRunFiles::setFileProblem(const QString & message)
{
  m_fileProblem = message;
  refreshValidator();
}


/**
* Save settings to the given group
* @param group :: The name of the group key to save to
*/
void MWRunFiles::saveSettings(const QString & group)
{
  QSettings settings;
  settings.beginGroup(group);

  settings.setValue("last_directory", m_lastDir);

  settings.endGroup();
}

/** 
* Set the file text
* @param text :: The text string to set
*/
void MWRunFiles::setFileText(const QString & text)
{
  m_uiForm.fileEditor->setText(text);
  findFiles();
}
/** 
* Puts the comma separated string in the fileEditor into the m_files array
*/
void MWRunFiles::findFiles()
{
  std::string text = this->m_uiForm.fileEditor->text().toStdString();
  if( text.empty() )
  {
    if( this->isOptional() )
    {
      setFileProblem("");
      m_foundFiles.clear();
      return;
    }
    else
    {
      setFileProblem("No files specified.");
      return;
    }
  }

  QStringList filestext = this->m_uiForm.fileEditor->text().split(", ", QString::SkipEmptyParts);
  QString file;

  Mantid::API::FileFinderImpl & fileSearcher = Mantid::API::FileFinder::Instance();
  std::vector<std::string> filenames;
  QString error("");
  try
  {
    if( isForRunFiles() )
    {
      filenames = fileSearcher.findRuns(text);
    }
    else
    {
      foreach(file, filestext)
      {
        std::string result = fileSearcher.getFullPath(file.toStdString());
        Poco::File test(result);
        if ( ( ! result.empty() ) && test.exists() )
        {
          filenames.push_back(file.toStdString());
        }
        else
        {
          throw std::invalid_argument("File \"" + file.toStdString() + "\" not found");
        }
      }
    }
  }
  catch(Exception::NotFoundError& exc)
  {
    error = QString::fromStdString(exc.what());
  }
  catch(std::invalid_argument& exc)
  {
    error = QString::fromStdString(exc.what());
  }
  if( !error.isEmpty() )
  {
    setFileProblem(error);
    return;
  }

  m_foundFiles.clear();
  for( size_t i = 0; i < filenames.size(); ++i)
  {
    m_foundFiles.append(QString::fromStdString(filenames[i]));
  }
  if( m_foundFiles.isEmpty() )
  {
    setFileProblem("Error: No files found. Check search paths and instrument selection.");
  }
  else if( m_foundFiles.count() > 1 && this->allowMultipleFiles() == false )
  {
    setFileProblem("Error: Multiple files specified.");
  }
  else
  {
    setFileProblem("");
  }
}/**
* Read settings from the given group
* @param group :: The name of the group key to retrieve data from
*/
void MWRunFiles::readSettings(const QString & group)
{
  QSettings settings;
  settings.beginGroup(group);

  m_lastDir = settings.value("last_directory", "").toString();

  if ( m_lastDir == "" )
  {
    QStringList datadirs = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories")).split(";", QString::SkipEmptyParts);
    if ( ! datadirs.isEmpty() ) m_lastDir = datadirs[0];
  }

  settings.endGroup();
}

/**
* Set a new file filter for the file dialog based on the given extensions
* @returns A string containing the file filter
*/
QString MWRunFiles::createFileFilter()
{
  QStringList fileExts;
  if( m_algorithmProperty.isEmpty() )
  {
    if ( !m_fileExtensions.isEmpty() )
    {
      fileExts = m_fileExtensions;
    }
    else
    {
      std::vector<std::string> exts = ConfigService::Instance().Facility().extensions();
      for( std::vector<std::string>::iterator ex= exts.begin(); ex != exts.end(); ++ex )
      {
        fileExts.append(QString::fromStdString(*ex));
      }
    }
  }
  else
  {
    QStringList elements = m_algorithmProperty.split("|");
    if( elements.size() == 2 )
    {
      fileExts = getFileExtensionsFromAlgorithm(elements[0], elements[1]);
    }
  }

  QString fileFilter;
  QStringListIterator itr(fileExts);
  if (itr.hasNext())
  {
    fileFilter += "Files (";
    while( itr.hasNext() )
    {
      QString ext = itr.next();
      fileFilter += "*" + ext.toLower() + " ";
      fileFilter += "*" + ext.toUpper();
      if( itr.hasNext() )
      {
        fileFilter += " ";
      }
    }
    fileFilter += ")";
  }
  fileFilter += ";;All Files (*.*)";
  return fileFilter;
}

/**
* Create a list of file extensions from the given algorithm
* @param algName :: The name of the algorithm
* @param propName :: The name of the property
* @returns A list of file extensions
*/
QStringList MWRunFiles::getFileExtensionsFromAlgorithm(const QString & algName, const QString &propName)
{
  Mantid::API::IAlgorithm_sptr algorithm = Mantid::API::AlgorithmManager::Instance().createUnmanaged(algName.toStdString());
  QStringList fileExts;
  if(!algorithm) return fileExts;
  algorithm->initialize();
  Property *prop = algorithm->getProperty(propName.toStdString());
  FileProperty *fileProp = dynamic_cast<FileProperty*>(prop);
  if( fileProp )
  {
    std::set<std::string> allowed = fileProp->allowedValues();
    std::set<std::string>::const_iterator iend = allowed.end();
    QString preferredExt = QString::fromStdString(fileProp->getDefaultExt());
    int index(0);
    for(std::set<std::string>::const_iterator it = allowed.begin(); it != iend; ++it)
    {
      if ( ! it->empty() )
      {
        QString ext = QString::fromStdString(*it);
        fileExts.append(ext);
        if( ext == preferredExt )
        {
          fileExts.move(index, 0);
        }
        ++index;
      }
    }
  }
  return fileExts;
}

/** Lauches a load file browser allowing a user to select multiple
*  files
*  @return the names of the selected files as a comma separated list
*/
QString MWRunFiles::openFileDia()
{
  QStringList filenames;
  QString dir = m_lastDir;

  if( m_fileFilter.isEmpty() )
  {
    m_fileFilter = createFileFilter();
  }

  if ( m_allowMultipleFiles )
  {
    filenames = QFileDialog::getOpenFileNames(this, "Open file", dir, m_fileFilter);
  }
  else
  {
    QString file = QFileDialog::getOpenFileName(this, "Open file", dir, m_fileFilter);
    if(  !file.isEmpty() ) filenames.append(file);
  }

  if(filenames.isEmpty())
  {
    return "";
  }
  m_lastDir = QFileInfo(filenames.front()).absoluteDir().path();
  // turns the QStringList into a coma separated list inside a QString
  return filenames.join(", ");
}


/** flag a problem with the supplied entry number, an empty string means no error.
*  file errors take precedence of these errors
*  @param message the message to display
*/
void MWRunFiles::setEntryNumProblem(const QString & message)
{
  m_entryNumProblem = message;
  refreshValidator();
}

/** Checks the data m_fileProblem and m_entryNumProblem to see if the validator label
*  needs to be displayed
*/
void MWRunFiles::refreshValidator()
{
  if ( ! m_fileProblem.isEmpty() )
  {
    m_uiForm.valid->setToolTip(m_fileProblem);
    m_uiForm.valid->show();
  }
  else if ( ! m_entryNumProblem.isEmpty() && m_doMultiEntry )
  {
    m_uiForm.valid->setToolTip(m_entryNumProblem);
    m_uiForm.valid->show();
  }
  else
  {
    m_uiForm.valid->hide();
  }
}

/** This slot opens a file browser
*/
void MWRunFiles::browseClicked()
{
  QString uFile = openFileDia();
  if( uFile.trimmed().isEmpty() ) return;

  if( this->allowMultipleFiles() )
  {
    if ( !m_uiForm.fileEditor->text().isEmpty() )
    {
      m_uiForm.fileEditor->setText(m_uiForm.fileEditor->text()+", " + uFile);
    }
    else
    {
      m_uiForm.fileEditor->setText(uFile);
    }
  }
  else
  {
    m_uiForm.fileEditor->setText(uFile);
  }
  emit fileEditingFinished();
}
/** Currently just checks that entryNum contains an int > 0 and hence might be a
*  valid entry number
*/
void MWRunFiles::checkEntry()
{
  if (m_uiForm.entryNum->text().isEmpty())
  {
    setEntryNumProblem("");
    return;
  }

  bool good;
  const int num = m_uiForm.entryNum->text().toInt(&good);
  if (! good )
  {
    setEntryNumProblem("The entry number must be an integer");
    return;
  }
  if ( num < 1 )
  {
    setEntryNumProblem("The entry number must be an integer > 0");
    return;
  }

  setEntryNumProblem("");
}
