#include "MantidQtMantidWidgets/MWRunFiles.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/LiveListenerFactory.h"

#include <QStringList>
#include <QFileDialog>
#include <QFileInfo>
#include <QHash>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QUrl>
#include <QtConcurrentRun>
#include <Poco/File.h>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

////////////////////////////////////////////////////////////////////
// FindFilesThread
////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 *
 * @param parent :: pointer to the parent QObject.
 */
FindFilesThread::FindFilesThread(QObject *parent) :
  QThread(parent), m_error(), m_filenames(), m_valueForProperty(), m_text(),
  m_algorithm(), m_property(), m_isForRunFiles(), m_isOptional()
{
}

/**
 * Set the values needed for the thread to run.
 *
 * @param text              :: the text containing the file names, typed in by the user
 * @param isForRunFiles     :: whether or not we are finding run files.
 * @param isOptional        :: whether or not the files are optional.
 * @param algorithmProperty :: the algorithm and property to use as an alternative to FileFinder.  Optional.
 */
void FindFilesThread::set(QString text, bool isForRunFiles, bool isOptional, const QString & algorithmProperty)
{
  m_text = text.trimmed().toStdString();
  m_isForRunFiles = isForRunFiles;
  m_isOptional = isOptional;

  QStringList elements = algorithmProperty.split("|");

  if( elements.size() == 2 )
  {
    m_algorithm = elements[0];
    m_property = elements[1];
  }
}

/**
 * Called when the thread is ran via start().  Tries to find the files, and
 * populates the error and filenames member variables with the result of the search.
 *
 * At present, there are two possible use cases:
 *
 * 1. Files are found directly by the FileFinder.  This is the default case.
 * 2. Files are found using the specified algorithm property.  In this case, a class user must have
 *    specified the algorithm and property via MWRunFiles::setAlgorithmProperty().
 */
void FindFilesThread::run()
{
  // Reset result member vars.
  m_error.clear();
  m_filenames.clear();
  m_valueForProperty.clear();

  if( m_text.empty() )
  {
    if( m_isOptional )
      m_error = "";
    else
      m_error = "No files specified.";

    return;
  }

  Mantid::API::FileFinderImpl & fileSearcher = Mantid::API::FileFinder::Instance();

  try
  {
    // Use the property of the algorithm to find files, if one has been specified.
    if( m_algorithm.length() != 0 && m_property.length() != 0 )
    {
      getFilesFromAlgorithm();
    }
    // Else if we are loading run files, then use findRuns.
    else if( m_isForRunFiles )
    {
      m_filenames = fileSearcher.findRuns(m_text);
      m_valueForProperty = "";
      for(auto cit = m_filenames.begin(); cit != m_filenames.end(); ++cit)
      {
        m_valueForProperty += QString::fromStdString(*cit) + ",";
      }
      m_valueForProperty.chop(1);
    }
    // Else try to run a simple parsing on the string, and find the full paths individually.
    else
    {
      // Tokenise on ","
      std::vector<std::string> filestext;
      filestext = boost::split(filestext, m_text, boost::is_any_of(","));

      // Iterate over tokens.
      auto it = filestext.begin();
      for( ; it != filestext.end(); ++it)
      {
        boost::algorithm::trim(*it);
        std::string result = fileSearcher.getFullPath(*it);
        Poco::File test(result);
        if ( ( ! result.empty() ) && test.exists() )
        {
          m_filenames.push_back(*it);
          m_valueForProperty += QString::fromStdString(*it) + ",";
        }
        else
        {
          throw std::invalid_argument("File \"" + (*it) + "\" not found");
        }
      }
      m_valueForProperty.chop(1);
    }
  }
  catch(std::exception& exc)
  {
    m_error = exc.what();
    m_filenames.clear();
  }
  catch(...)
  {
    m_error = "An unknown error occurred while trying to locate the file(s). Please contact the development team";
    m_filenames.clear();
  }
}

/**
 * Create a list of files from the given algorithm property.
 */
void FindFilesThread::getFilesFromAlgorithm()
{
  Mantid::API::IAlgorithm_sptr algorithm = Mantid::API::AlgorithmManager::Instance().createUnmanaged(m_algorithm.toStdString());

  if(!algorithm) throw std::invalid_argument("Cannot create algorithm " + m_algorithm.toStdString() + ".");

  algorithm->initialize();
  const std::string propName = m_property.toStdString();
  algorithm->setProperty(propName, m_text);

  Property *prop = algorithm->getProperty(propName);
  m_valueForProperty = QString::fromStdString(prop->value());
  
  FileProperty *fileProp = dynamic_cast<FileProperty*>(prop);
  MultipleFileProperty *multiFileProp = dynamic_cast<MultipleFileProperty*>(prop);

  if( fileProp )
  {
    m_filenames.push_back(fileProp->value());
  }
  else if( multiFileProp )
  {
    // This flattens any summed files to a set of single files so that you lose the information about
    // what was summed
    std::vector<std::vector<std::string> > filenames = algorithm->getProperty(propName);
    std::vector<std::string> flattenedNames = MultipleFileProperty::flattenFileNames(filenames);

    for( auto filename = flattenedNames.begin(); filename != flattenedNames.end(); ++filename )
    {
      m_filenames.push_back( *filename );
    }
  }

}

////////////////////////////////////////////////////////////////////
// MWRunFiles
////////////////////////////////////////////////////////////////////

MWRunFiles::MWRunFiles(QWidget *parent) 
  : MantidWidget(parent), m_findRunFiles(true), m_allowMultipleFiles(false), 
    m_isOptional(false), m_multiEntry(false), m_buttonOpt(Text), m_fileProblem(""),
    m_entryNumProblem(""), m_algorithmProperty(""), m_fileExtensions(), m_extsAsSingleOption(true),
    m_liveButtonState(Hide), m_foundFiles(), m_lastFoundFiles(), m_lastDir(), m_fileFilter()
{
  m_thread = new FindFilesThread(this);
  
  m_uiForm.setupUi(this);

  connect(m_uiForm.fileEditor, SIGNAL(textChanged(const QString &)), this, SIGNAL(fileTextChanged(const QString&)));
  connect(m_uiForm.fileEditor, SIGNAL(editingFinished()), this, SIGNAL(fileEditingFinished()));
  connect(m_uiForm.browseBtn, SIGNAL(clicked()), this, SLOT(browseClicked()));
  connect(m_uiForm.browseIco, SIGNAL(clicked()), this, SLOT(browseClicked()));

  connect(this, SIGNAL(fileEditingFinished()), this, SLOT(findFiles()));
  connect(m_uiForm.entryNum, SIGNAL(textChanged(const QString &)), this, SLOT(checkEntry()));
  connect(m_uiForm.entryNum, SIGNAL(editingFinished()), this, SLOT(checkEntry()));

  connect(m_thread, SIGNAL(finished()), this, SLOT(inspectThreadResult()));
  connect(m_thread, SIGNAL(finished()), this, SIGNAL(fileFindingFinished()));

  m_uiForm.fileEditor->clear();
  
  if (m_multiEntry)
  {
    m_uiForm.entryNum->show();
    m_uiForm.numEntries->show();
  }
  else
  {
    m_uiForm.entryNum->hide();
    m_uiForm.numEntries->hide();
  }

  doButtonOpt(m_buttonOpt);

  liveButtonState(m_liveButtonState);
  connect(this, SIGNAL(liveButtonSetEnabledSignal(bool)), m_uiForm.liveButton, SLOT(setEnabled(bool)));
  connect(this, SIGNAL(liveButtonSetEnabledSignal(bool)), m_uiForm.liveButton, SLOT(show()));
  connect(m_uiForm.liveButton, SIGNAL(toggled(bool)), this, SIGNAL(liveButtonPressed(bool)));

  setFocusPolicy(Qt::StrongFocus);
  setFocusProxy(m_uiForm.fileEditor);

  // When first used try to starting directory better than the directory MantidPlot is installed in
  // First try default save directory
  m_lastDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));

  // If that fails pick the first data search directory
  if(m_lastDir.isEmpty())
  {
    QStringList dataDirs = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories")).split(";", QString::SkipEmptyParts);

    if(!dataDirs.isEmpty())
      m_lastDir = dataDirs[0];
  }

  //this for accepts drops, but the underlying text input does not.
  this->setAcceptDrops(true);
  m_uiForm.fileEditor->setAcceptDrops(false);
}

MWRunFiles::~MWRunFiles() 
{
  // Before destruction, make sure the file finding thread has stopped running. Wait if necessary.
  m_thread->exit(-1);
  m_thread->wait();
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
* @param mode :: True if this widget searches for run files, false otherwise
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
  m_uiForm.textLabel->setVisible( ! text.isEmpty() );
}

/** Set the minimum width on the label widget
 *  @param width The new minimum width of the widget
 */
void MWRunFiles::setLabelMinWidth(const int width)
{
  m_uiForm.textLabel->setMinimumWidth(width);
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
* Returns the preference for how the dialog control should be
* @return the setting
*/
MWRunFiles::ButtonOpts MWRunFiles::doButtonOpt() const
{
  return m_buttonOpt;
}

/**
* Set how the browse should appear
* @param buttonOpt the preference for the control, if there will be one, to activate the dialog box
*/
void MWRunFiles::doButtonOpt(const MWRunFiles::ButtonOpts buttonOpt)
{
  m_buttonOpt = buttonOpt;
  if (buttonOpt == None)
  {
    m_uiForm.browseBtn->hide();
    m_uiForm.browseIco->hide();
  }
  else if (buttonOpt == Text)
  {
    m_uiForm.browseBtn->show();
    m_uiForm.browseIco->hide();
  }
  else if (buttonOpt == Icon)
  {
    m_uiForm.browseBtn->hide();
    m_uiForm.browseIco->show();
  }
}

/**
* Whether to find the number of entries in the file or assume (the
* normal situation) of one entry
* @return true if the widget is to look for multiple entries
*/
bool MWRunFiles::doMultiEntry() const
{
  return m_multiEntry;
}

/**
* Set to true to enable the period number box
* @param multiEntry whether to show the multiperiod box
*/
void MWRunFiles::doMultiEntry(const bool multiEntry)
{
  m_multiEntry = multiEntry;
  if (m_multiEntry)
  {
    m_uiForm.entryNum->show();
    m_uiForm.numEntries->show();
  }
  else
  {
    m_uiForm.entryNum->hide();
    m_uiForm.numEntries->hide();
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
* @param text :: The name of the algorithm and property in the form [AlgorithmName|PropertyName]
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
 * Returns whether the file dialog should display the exts as a single list or as multiple items
 * @return boolean
 */
bool MWRunFiles::extsAsSingleOption() const
{
  return m_extsAsSingleOption;
}

/**
 * Sets whether the file dialog should display the exts as a single list or as multiple items
 * @param value :: If true the file dialog wil contain a single entry will all filters
 */
void MWRunFiles::extsAsSingleOption(const bool value)
{
  m_extsAsSingleOption = value;
}

/// Returns whether the live button is being shown;
MWRunFiles::LiveButtonOpts MWRunFiles::liveButtonState() const
{
  return m_liveButtonState;
}

void MWRunFiles::liveButtonState(const LiveButtonOpts option)
{
  m_liveButtonState = option;
  if ( m_liveButtonState == Hide )
  {
    m_uiForm.liveButton->hide();
  }
  else
  {
    liveButtonSetEnabled(false); // This setting ensures right outcome if the connection check fails
    // Checks (asynchronously) whether it's possible to connect to the user's default instrument
    QtConcurrent::run(this, &MWRunFiles::checkLiveConnection);
    if ( m_liveButtonState == AlwaysShow )
    {
      m_uiForm.liveButton->show();
    }
  }
}

void MWRunFiles::checkLiveConnection()
{
  // Checks whether it's possible to connect to the user's default instrument
  if ( LiveListenerFactory::Instance().checkConnection(ConfigService::Instance().getInstrument().name()) )
  {
    emit liveButtonSetEnabledSignal(true);
  }
}

void MWRunFiles::liveButtonSetEnabled(const bool enabled)
{
  m_uiForm.liveButton->setEnabled(enabled);
}

void MWRunFiles::liveButtonSetChecked(const bool checked)
{
  m_uiForm.liveButton->setChecked(checked);
}

bool MWRunFiles::liveButtonIsChecked() const
{
  return m_uiForm.liveButton->isChecked();
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
 * Is the widget currently searching
 * @return True if a search is inprogress
 */
bool MWRunFiles::isSearching() const
{
  return (m_thread ? m_thread->isRunning() : false);
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


/** Check if any text, valid or not, has been entered into the line edit
*  @return true if no text has been entered
*/
bool MWRunFiles::isEmpty() const
{
  return m_uiForm.fileEditor->text().isEmpty();
}

/** The verbatum, unexpanded text, that was entered into the box
*  @return the contents shown in the Line Edit
*/
QString MWRunFiles::getText() const
{
  return m_uiForm.fileEditor->text();
}

/** The number the user entered into the entryNum lineEdit
* or NO_ENTRY_NUM on error. Checking if isValid is true should
* eliminate the possiblity of getting NO_ENTRY_NUM
*/
int MWRunFiles::getEntryNum() const
{
  if ( m_uiForm.entryNum->text().isEmpty() || (! m_multiEntry) )
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

/** Set the entry displayed in the box to this value
* @param num the period number to use
*/
void MWRunFiles::setEntryNum(const int num)
{
  m_uiForm.entryNum->setText(QString::number(num));
}

/**
 * Retrieve user input from this widget. This expands the current
 * file text to include the found absolute paths so that no more
 * searching is required
 * NOTE: This knows nothing of periods yet
 * @returns A QVariant containing the text string for the algorithm property
 */
QVariant MWRunFiles::getUserInput() const
{
  return QVariant(m_thread->valueForProperty());
}

/**
 * "Silently" sets the value of the widget.  It does NOT emit a signal to say it
 * has changed, and as far as the file finding routine is concerned it has not
 * been modified and so it will NOT go searching for files.
 *
 * @param value A QString containing text to be entered into the widget
 */

void MWRunFiles::setText(const QString & value)
{
  m_uiForm.fileEditor->setText(value);
}

/**
 * Sets a value on the widget through a common interface. The 
 * QVariant is assumed to be text and to contain a file string. Note that this
 * is primarily here for use within the AlgorithmDialog.
 *
 * Emits fileEditingFinished(), and changes to state of the text box widget
 * to be "modified", so that the file finder will try and find the file(s).
 *
 * @param value A QVariant containing user text
 */
void MWRunFiles::setUserInput(const QVariant & value)
{
  m_uiForm.fileEditor->setText(value.toString());
  m_uiForm.fileEditor->setModified(true);
  emit fileEditingFinished(); // Which is connected to slot findFiles()
}

/**
 * Flag a problem with the file the user entered, an empty string means no error but
 * there may be an error with the entry box if enabled. Errors passed here are shown first
 * @param message :: A message to include or "" for no error
 */
void MWRunFiles::setFileProblem(const QString & message)
{
  m_fileProblem = message;
  refreshValidator();
}

/**
* Return the error.
* @returns A string explaining the error.
*/
QString MWRunFiles::getFileProblem()
{
  return m_fileProblem;
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

/** Writes the total number of periods in a file to the NumEntries
*  Qlabel
*  @param number the number to write, if this is < 1 a ? will be displayed in it's place
*/
void MWRunFiles::setNumberOfEntries(const int number)
{
  const QString total = number > 0 ? QString::number(number) : "?";
  {
    m_uiForm.numEntries->setText("/"+total);
  }
}

/** Inform the widget of a running instance of MonitorLiveData to be used in stopLiveListener().
 *  Note that the type passed in is IAlgorithm and that no check is made that it actually refers
 *  to an instance of MonitorLiveData.
 *  @param monitorLiveData The running algorithm
 */
void MWRunFiles::setLiveAlgorithm(const IAlgorithm_sptr& monitorLiveData)
{
  m_monitorLiveData = monitorLiveData;
}

/**
 * Gets the instrument currently set by the override property.
 *
 * If no override is set then the instrument set by default instrument configurtion
 * option will be used and this function returns an empty string.
 *
 * @return Name of instrument, empty if not set
 */
QString MWRunFiles::getInstrumentOverride()
{
  return m_defaultInstrumentName;
}

/**
 * Sets an instrument to fix the widget to.
 *
 * If an instrument name is geven then the widget will only look for files for that
 * instrument, providing na empty string will remove this restriction and will search
 * using the default instrument.
 *
 * @param instName Name of instrument, empty to disable override
 */
void MWRunFiles::setInstrumentOverride(const QString & instName)
{
  m_defaultInstrumentName = instName;
}

/** 
* Set the file text.  This is different to setText in that it emits findFiles, as well
* changing the state of the text box widget to "modified = true" which is a prerequisite
* of findFiles.
*
* @param text :: The text string to set
*/
void MWRunFiles::setFileTextWithSearch(const QString & text)
{
  setFileTextWithoutSearch(text);
  findFiles();
}
/**
* Set the file text but do not search
* @param text :: The text string to set
*/
void MWRunFiles::setFileTextWithoutSearch(const QString & text)
{
  m_uiForm.fileEditor->setText(text);
  m_uiForm.fileEditor->setModified(true);
}

/**
* Finds the files specified by the user in a background thread.
*/
void MWRunFiles::findFiles()
{
  if(m_uiForm.fileEditor->isModified())
  {
    // Reset modified flag.
    m_uiForm.fileEditor->setModified(false);

    // If the thread is running, cancel it.
    if( m_thread->isRunning() )
    {
      m_thread->exit(-1);
      m_thread->wait();
    }

    emit findingFiles();

    // Set the values for the thread, and start it running.
    QString searchText = m_uiForm.fileEditor->text();

    // If we have an override instrument then add it in appropriate places to the search text
    if (!m_defaultInstrumentName.isEmpty())
    {
      // Regex to match a selection of run numbers as defined here: mantidproject.org/MultiFileLoading
      // Also allowing spaces between delimiters as this seems to work fine
      boost::regex runNumbers("([0-9]+)([:+-] ?[0-9]+)? ?(:[0-9]+)?", boost::regex::extended);
      // Regex to match a list of run numbers delimited by commas
      boost::regex runNumberList("([0-9]+)(, ?[0-9]+)*", boost::regex::extended);

      // See if we can just prepend the instrument and be done
      if (boost::regex_match(searchText.toStdString(), runNumbers))
      {
        searchText = m_defaultInstrumentName + searchText;
      }
      // If it is a list we need to prepend the instrument to all run numbers
      else if (boost::regex_match(searchText.toStdString(), runNumberList))
      {
        QStringList runNumbers = searchText.split(",", QString::SkipEmptyParts);
        QStringList newRunNumbers;

        for(auto it = runNumbers.begin(); it != runNumbers.end(); ++it)
          newRunNumbers << m_defaultInstrumentName + (*it).simplified();

        searchText = newRunNumbers.join(",");
      }
    }

    m_thread->set(searchText, isForRunFiles(), this->isOptional(), m_algorithmProperty);
    m_thread->start();
  }
  else
  {
    // Make sure errors are correctly set if we didn't run
    inspectThreadResult();
  }
}

/** Calls cancel on a running instance of MonitorLiveData.
 *  Requires that a handle to the MonitorLiveData instance has been set via setLiveListener()
 *  @return A handle to the cancelled algorithm (usable if the method is called directly)
 */
IAlgorithm_const_sptr MWRunFiles::stopLiveAlgorithm()
{
  IAlgorithm_const_sptr theAlgorithmBeingCancelled = m_monitorLiveData;
  if ( m_monitorLiveData && m_monitorLiveData->isRunning() )
  {
    m_monitorLiveData->cancel();
    m_monitorLiveData.reset();
  }
  return theAlgorithmBeingCancelled;
}

/**
 * Called when the file finding thread finishes.  Inspects the result
 * of the thread, and emits fileFound() if it has been successful.
 */
void MWRunFiles::inspectThreadResult()
{
  // Get results from the file finding thread.
  std::string error = m_thread->error();
  std::vector<std::string> filenames = m_thread->filenames();

  if( !error.empty() )
  {
    setFileProblem(QString::fromStdString(error));
    return;
  }

  m_lastFoundFiles = m_foundFiles;
  m_foundFiles.clear();

  for( size_t i = 0; i < filenames.size(); ++i)
  {
    m_foundFiles.append(QString::fromStdString(filenames[i]));
  }
  if( m_foundFiles.isEmpty() && !isOptional() )
  {
    setFileProblem("No files found. Check search paths and instrument selection.");
  }
  else if( m_foundFiles.count() > 1 && this->allowMultipleFiles() == false )
  {
    setFileProblem("Multiple files specified.");
  }
  else
  {
    setFileProblem("");
  }

  // Only emit the signal if file(s) were found
  if ( ! m_foundFiles.isEmpty() ) emit filesFound();
  if ( m_lastFoundFiles != m_foundFiles ) emit filesFoundChanged();
}

/**
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
    else if( isForRunFiles() )
    {
      std::vector<std::string> exts = ConfigService::Instance().getFacility().extensions();
      for( std::vector<std::string>::iterator ex= exts.begin(); ex != exts.end(); ++ex )
      {
        fileExts.append(QString::fromStdString(*ex));
      }
    }
    else {}
  }
  else
  {
    QStringList elements = m_algorithmProperty.split("|");
    if( elements.size() == 2 )
    {
      fileExts = getFileExtensionsFromAlgorithm(elements[0], elements[1]);
    }
  }

  QString allFiles("All Files (*.*)");
  if( !fileExts.isEmpty() )
  {
    
    // The list may contain upper and lower cased versions, ensure these are on the same line
    // I want this ordered
    QList<QPair<QString, QStringList> > finalIndex;
    QStringListIterator sitr(fileExts);
    QString ext = sitr.next();
    finalIndex.append(qMakePair(ext.toUpper(), QStringList(ext)));
    while( sitr.hasNext() )
    {
      ext = sitr.next();
      QString key = ext.toUpper();
      bool found(false);
      const int itemCount = finalIndex.count();
      for( int i = 0 ; i < itemCount; ++i )
      {
        if( key == finalIndex[i].first )
        {
          finalIndex[i].second.append(ext);
          found = true;
          break;
        }
      }
      if( !found )
      {
        finalIndex.append(qMakePair(key, QStringList(ext)));
      }
    }

    // The file filter consists of three parts, which we will combine to create the
    // complete file filter:
    QString dataFiles("Data Files (");
    QString individualFiles("");

    if( extsAsSingleOption() )
    {
      QListIterator<QPair<QString, QStringList> > itr(finalIndex);
      while( itr.hasNext() )
      {
        const QStringList values = itr.next().second;
        
        individualFiles += "*" + values.join(" *") + ";;";
        dataFiles += "*" + values.join(" *") + " ";
      }
      //Don't remove final ;; from individualFiles as we are going to tack on allFiles anyway
      dataFiles.chop(1); // Remove last space
      dataFiles += ");;";
    }
    else
    {
      QListIterator<QPair<QString, QStringList> > itr(finalIndex);
      while( itr.hasNext() )
      {
        const QStringList values = itr.next().second;
        dataFiles += "*" + values.join(" *") + ";;";
      }
    }
    return dataFiles + individualFiles + allFiles;
  }
  else
  {
    return allFiles;
  }
  
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
  MultipleFileProperty *multiFileProp = dynamic_cast<MultipleFileProperty*>(prop);

  std::vector<std::string> allowed;
  QString preferredExt;

  if( fileProp )
  {
    allowed = fileProp->allowedValues();
    preferredExt = QString::fromStdString(fileProp->getDefaultExt());
  }
  else if( multiFileProp )
  {
    allowed = multiFileProp->allowedValues();
    preferredExt = QString::fromStdString(multiFileProp->getDefaultExt());
  }
  else
  {
    return fileExts;
  }

  std::vector<std::string>::const_iterator iend = allowed.end();
  int index(0);
  for(std::vector<std::string>::const_iterator it = allowed.begin(); it != iend; ++it)
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

  return fileExts;
}

/** Lauches a load file browser allowing a user to select multiple
*  files
*  @return the names of the selected files as a comma separated list
*/
QString MWRunFiles::openFileDialog()
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
  else if ( ! m_entryNumProblem.isEmpty() && m_multiEntry )
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
  QString uFile = openFileDialog();
  if( uFile.trimmed().isEmpty() ) return;

  m_uiForm.fileEditor->setText(uFile);
  m_uiForm.fileEditor->setModified(true);

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

/**
  * Called when an item is dropped
  * @param de :: the drop event data package
  */
void MWRunFiles::dropEvent(QDropEvent *de)
{
  const QMimeData *mimeData = de->mimeData(); 
  if (mimeData->hasUrls()){
    auto url_list = mimeData->urls(); 
    m_uiForm.fileEditor->setText(url_list[0].toLocalFile());
    de->acceptProposedAction();
  }else if (mimeData->hasText()){
    QString text = mimeData->text();
    m_uiForm.fileEditor->setText(text); 
    de->acceptProposedAction();
  }
  
}

/**
  * Called when an item is dragged onto a control
  * @param de :: the drag event data package
  */
void MWRunFiles::dragEnterEvent(QDragEnterEvent *de)
{
  const QMimeData *mimeData = de->mimeData();  
  if (mimeData->hasUrls()){
    auto listurl = mimeData->urls(); 
    if (listurl.empty())
      return;
    if (!listurl[0].isLocalFile())
      return;
    de->acceptProposedAction();
  }
  else if(mimeData->hasText()) 
  {
    QString text = mimeData->text();
    if (text.contains(" = mtd[\""))
      de->setDropAction(Qt::IgnoreAction);
    else
      de->acceptProposedAction();
  }
}
