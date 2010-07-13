#include "MantidQtMantidWidgets/MWRunFiles.h"
#include "MantidKernel/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include <QStringList>
#include <QFileDialog>
#include <QFileInfo>
#include "boost/lexical_cast.hpp"
#include "Poco/StringTokenizer.h"

using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;

static const int MAX_FILE_NOT_FOUND_DISP = 3;

MWRunFiles::MWRunFiles(QWidget *parent) : MantidWidget(parent), m_allowMultipleFiles(false), 
  m_isOptional(false), m_fileFilter("")
{
  m_uiForm.setupUi(this);
  
  const std::vector<std::string>& searchDirs = ConfigService::Instance().getDataSearchDirs();
  if ( searchDirs.size() > 0 )
  {
    m_defDir = QString::fromStdString(searchDirs.front());
  }

  setupInstrumentNameIndex();
  connect(m_uiForm.fileEditor, SIGNAL(editingFinished()), this, SLOT(readEntries()));
  connect(m_uiForm.browseBtn, SIGNAL(clicked()), this, SLOT(browseClicked()));
  m_uiForm.fileEditor->clear();
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
 * @param text A string giving the label to use for the text
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
 * @param allow If true then the widget will accept multiple files else only a single file may be specified
 */
void MWRunFiles::allowMultipleFiles(const bool allow)
{
  m_allowMultipleFiles = allow;
  readEntries();
}

/**
 * Return whether empty input is allowed
 */
bool MWRunFiles::isOptional() const
{
  return m_isOptional;
}

void MWRunFiles::isOptional(const bool optional)
{
  m_isOptional = optional;
  readEntries();
}

/**
 * Set a new file filter for the file dialog based on the given extensions
 * @param fileExts A list of file extensions with with to create the filter
 */
void MWRunFiles::setExtensionList(const QStringList & fileExts)
{
  m_fileFilter = "Files (";
  QStringListIterator itr(fileExts);
  while( itr.hasNext() )
  {
    QString ext = itr.next();
    m_fileFilter += "*" + ext.toLower() + " ";
    m_fileFilter += "*" + ext.toUpper();
    if( itr.hasNext() )
    {
      m_fileFilter += " ";
    }
  }
  m_fileFilter += ");; All Files (*.*)";
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

/** Returns the user entered filenames as a comma seperated list (integers are expanded to
*  filenames)
*  @return an array of filenames entered in the box
*/
const std::vector<std::string>& MWRunFiles::getFileNames() const
{
  return m_files;
}

/** Safer than using getRunFiles()[0] in the situation were there are no files
*  @return an empty string is returned if no input files have been defined or one of the files can't be found, otherwise it's the name of the first input file
*  @throw invalid_argument if one of the files couldn't be found it then no filenames are returned
*/
QString MWRunFiles::getFile1() const
{
  const std::vector<std::string>& fileList = getFileNames();
  if ( fileList.begin() != fileList.end() )
  {
    return QString::fromStdString(*(fileList.begin()));
  }
  return "";
}

/**
 * Save settings to the given group
 * @param group The name of the group key to save to
 */
void MWRunFiles::saveSettings(const QString & group)
{
  QSettings settings;
  settings.beginGroup(group);
  
  settings.setValue("last_directory", m_lastDir);
  
  settings.endGroup();
}

/**
 * Read settings from the given group
 * @param group The name of the group key to retrieve data from
 */
void MWRunFiles::readSettings(const QString & group)
{
  QSettings settings;
  settings.beginGroup(group);
  
  m_lastDir = settings.value("last_directory", "").toString();
  
  settings.endGroup();
}

/** Lauches a load file browser allowing a user to select multiple
*  files
*  @return the names of the selected files as a comma separated list
*/
QString MWRunFiles::openFileDia()
{
  QStringList filenames;
  QString dir = m_lastDir;
  
  filenames = QFileDialog::getOpenFileNames(this, "Open file", dir, m_fileFilter);
  if(filenames.isEmpty())
  {
	  return "";
  }
  m_lastDir = QFileInfo(filenames.front()).absoluteDir().path();
  // turns the QStringList into a coma separated list inside a QString
  return filenames.join(", ");
}
/// Convert integers from the LineEdit box into filenames but leaves all non-integer values untouched
void MWRunFiles::readRunNumAndRanges()
{
  readCommasAndHyphens(m_uiForm.fileEditor->text().toStdString(), m_files);
  // only show the validator if errors are found below
  m_uiForm.valid->hide();
  QStringList errors;

  // set up the object we use for validation
  std::vector<std::string> exts;
  exts.push_back("raw"); exts.push_back("RAW");
  exts.push_back("NXS"); exts.push_back("nxs");
  FileProperty loadData("Filename", "", FileProperty::Load, exts);

  if ( m_files.empty() && !isOptional() )
  {
    errors << "A filename is required.";
  }
  
  std::vector<std::string>::iterator i = m_files.begin(), end = m_files.end();
  //
  // @todo MG: There was no padding originally so this was added to work for TS1 and TS2 at ISIS
  // but a uch more general solution is necessary.
  //
  for ( ; i != end; ++i )
  {
    std::string run_str = *i;
    bool num_only(true);
  try
	{
    boost::lexical_cast<unsigned int>(*i);
	}
	catch ( boost::bad_lexical_cast &)
	{// the entry doesn't read as a run number
    num_only = false;
	}// the alternative is that they entered a filename and we leave that as it is

  if( num_only )
  {
    // First pad to 5
    int ndigits = static_cast<int>(run_str.size());
    if( ndigits != 5 )
    {
      run_str.insert(0, 5 - ndigits, '0');
    }
    std::string problem = loadData.setValue(m_instrPrefix.toStdString() + run_str + ".raw");
    if( !problem.empty() )
    {
      //Try 8
      run_str.insert(0, 3, '0');
    }
    *i = m_instrPrefix.toStdString() + run_str + ".raw"; 
  }
  if ( errors.size() < MAX_FILE_NOT_FOUND_DISP )
	{
    std::string problem = loadData.setValue(*i);
	  if ( !problem.empty() )
	  {
	    errors << QString::fromStdString(problem);
	  }
	}
  }
  if ( errors.size() > 0 )
  {
    if ( m_files.size() > 0 )
	{
      m_uiForm.valid->setToolTip(errors.join(",\n")+"\nHas the path been entered into your Mantid.user.properties file?");
	}
	else
	{
      m_uiForm.valid->setToolTip(errors.front());
	}
	m_uiForm.valid->show();
	m_files.clear();
  }
}
void MWRunFiles::readCommasAndHyphens(const std::string &in, std::vector<std::string> &out)
{
  out.clear();
  if ( in.empty() )
  {// it is not an error to have an empty line but it would cause problems with an error check a the end of this function
    return;
  }
  
  Poco::StringTokenizer ranges(in, "-");
  Poco::StringTokenizer::Iterator aList = ranges.begin();

  do
  {
    Poco::StringTokenizer preHyp(*aList, ",", Poco::StringTokenizer::TOK_TRIM);
    Poco::StringTokenizer::Iterator readPos = preHyp.begin();
    if ( readPos == preHyp.end() )
    {// can only mean that there was only an empty string or white space the '-'
      throw std::invalid_argument("'-' found at the start of a list, can't interpret range specification");
    }
	  out.reserve(out.size()+preHyp.count());
    for ( ; readPos != preHyp.end(); ++readPos )
    {
      out.push_back(*readPos);
    }
    if (aList == ranges.end()-1)
    {// there is no more input
      break;
    }

    Poco::StringTokenizer postHy(*(aList+1),",",Poco::StringTokenizer::TOK_TRIM);
    readPos = postHy.begin();
    if ( readPos == postHy.end() )
    {
      throw std::invalid_argument("A '-' follows straight after another '-', can't interpret range specification");
    }
	
    try
    {//we've got the point in the string where there was a hyphen, first get the stuff that is after it
      // the tokenizer will always return at least on string
      const int rangeEnd = boost::lexical_cast<int>(*readPos);
      // the last string before the hyphen should be an int, here we get the it
      const int rangeStart = boost::lexical_cast<int>(out.back()); 
      // counting down isn't supported
      if ( rangeStart > rangeEnd )
      {
        throw std::invalid_argument("A range where the first integer is larger than the second is not allowed");
      }
      // expand the range
      for ( int j = rangeStart+1; j < rangeEnd; j++ )
      {
        out.push_back(boost::lexical_cast<std::string>(j));
      }
	  }
	  catch (boost::bad_lexical_cast)
	  {// the hyphen wasn't between two numbers, don't intepret it as a range instaed reconstruct the hyphenated string and add it to the list
	    out.back() += "-" + *readPos;
	  }
  }
  while( ++aList != ranges.end() );

  if ( *(in.end()-1) == '-' )
  {
    throw std::invalid_argument("'-' found at the end of a list, can't interpret range specification");
  }
}
/** This slot opens a file browser, then adds any selected file to the
*  fileEditor LineEdit and emits fileChanged()
*/
void MWRunFiles::browseClicked()
{
  QString uFile = openFileDia();
  if( uFile.trimmed().isEmpty() ) return;

  if ( ! m_uiForm.fileEditor->text().isEmpty() )
  {
    m_uiForm.fileEditor->setText(
	  m_uiForm.fileEditor->text()+", " + uFile);
  }
  else m_uiForm.fileEditor->setText(uFile);
  
  readEntries();
}
void MWRunFiles::instrumentChange(const QString & instrName)
{
  //Lookup for a prefix from a name.
  if( m_instrNameIndex.contains(instrName) )
  {
    m_instrPrefix = m_instrNameIndex.value(instrName);
  }
  //Use the name as-is if no name is mapped to it
  else
  {
    m_instrPrefix = instrName;
  }
  
  // the tooltip will tell users which instrument, if any, if associated with this control
  if (m_instrPrefix.isEmpty())
  {
    setToolTip("(no intrument selected)");
  }
  else
  {
    setToolTip("Searching for files using prefix " + m_instrPrefix);
  }
  
  readEntries();
}

/** 
 * Puts the comma separated string in the fileEditor into the m_files array
*/
void MWRunFiles::readEntries()
{
  readRunNumAndRanges();
  emit fileChanged();
}

/**
 * Setup the instrument name->prefix lookup index
 */
void MWRunFiles::setupInstrumentNameIndex()
{
  m_instrNameIndex.clear();
 	Mantid::Kernel::ConfigServiceImpl & mtd_config = Mantid::Kernel::ConfigService::Instance();
	// get instrument names from config file
	std::string key_name = std::string("instrument.names.") + mtd_config.getString("default.facility");
	QString names = QString::fromStdString(mtd_config.getString(key_name));
	QStringList nameList = names.split(";", QString::SkipEmptyParts);
	// get instrument prefixes from config file
	std::string key_pref = std::string("instrument.prefixes.") + mtd_config.getString("default.facility");
	QString prefixes = QString::fromStdString(mtd_config.getString(key_pref));
	QStringList prefList = prefixes.split(";", QString::SkipEmptyParts);

  int nameCount = nameList.count();
  if( nameCount != prefList.count() )
  {
    throw std::runtime_error("Size mismatch with name/prefix lists. Check that the instrument.names.[facility] and instrument.prefixes.[facility] have the same length.");
  }

  for( int i = 0; i < nameCount; ++i )
  {
    m_instrNameIndex.insert(nameList[i], prefList[i]);
  }
}

//****************************************************************************
//    MWRunFile
//****************************************************************************

MWRunFile::MWRunFile(QWidget *parent):
  MWRunFiles(parent), m_userChange(false)
{
}
/** Lauches a load file browser where a user can select only
*  a single file
*  @return the name of the file that was selected
*/
QString MWRunFile::openFileDia()
{
  QString filename;
  QString dir = m_lastDir;
  
  filename = QFileDialog::getOpenFileName(this, "Open file", dir, m_fileFilter);
  if( ! filename.isEmpty() )
  {
  	m_lastDir = QFileInfo(filename).absoluteDir().path();
  }
  return filename;
}

/** 
* Slot changes the filename only if it has not already been changed by
* user
*/
void MWRunFile::suggestFilename(const QString &newName)
{
  try
  {// check if the user has entered another value (other than the default) into the box
    m_userChange =
      m_userChange || (m_suggestedName != m_uiForm.fileEditor->text());
  }
  catch (std::invalid_argument&)
  {// its possible that the old value was a bad value and caused an exception, ignore that
  }
  //only set the filename to the default if the user hadn't changed it
  if ( ! m_userChange )
  {
    m_suggestedName = newName;
    m_uiForm.fileEditor->setText(newName);
    readEntries();
  }
}
/** Slot opens a file browser, writing the selected file to fileEditor
*  LineEdit and emits fileChanged()
*/
void MWRunFile::browseClicked()
{
  QString uFile = openFileDia();
  if( uFile.trimmed().isEmpty() ) return;

  m_uiForm.fileEditor->setText(uFile);
  
  readEntries();
}

void MWRunFile::readEntries()
{
  MWRunFiles::readEntries();

  if ( m_files.size() > 1 )
  {
    m_uiForm.valid->setToolTip("Only one file is allowed here (, and - are not\nallowed in filenames)");
	m_uiForm.valid->show();
	m_files.clear();
  }
  
  emit fileChanged();
}
