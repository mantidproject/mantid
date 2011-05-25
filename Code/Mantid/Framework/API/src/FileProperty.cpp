//-----------------------------------------------------------------
// Includes
//-----------------------------------------------------------------
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FileFinder.h"
#include "MantidKernel/FileValidator.h"
#include "MantidKernel/DirectoryValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include <Poco/Path.h>
#include <Poco/File.h>
#include <cctype>
#include <algorithm>

namespace Mantid
{

namespace API
{

using namespace Mantid::Kernel;



//-----------------------------------------------------------------
// Public member functions
//-----------------------------------------------------------------
/**
 * Constructor
 * @param name ::          The name of the property
 * @param default_value :: A default value for the property
 * @param exts ::          The allowed extensions, the front entry in the vector will be the default extension
 * @param action ::        An enum indicating whether this should be a load/save property
 * @param direction ::     An optional direction (default=Input)
 */
FileProperty::FileProperty(const std::string & name, const std::string& default_value, unsigned int action,
         const std::vector<std::string> & exts, unsigned int direction)
  : PropertyWithValue<std::string>(name, default_value,
      /* Create either a FileValidator or a DirectoryValidator, depending on Action */
      (action == FileProperty::Directory || action == FileProperty::OptionalDirectory) ?
          new DirectoryValidator(action == FileProperty::Directory) :
          new FileValidator(exts, (action == FileProperty::Load) )
      , direction),
    m_action(action),
    m_defaultExt(""),
    m_runFileProp(false)
{
  setUp((exts.size() > 0) ? exts.front() : "");
}



/**
 * Constructor
 * @param name ::          The name of the property
 * @param default_value :: A default value for the property
 * @param ext ::           The allowed extension
 * @param action ::        An enum indicating whether this should be a load/save property
 * @param direction ::     An optional direction (default=Input)
 */
FileProperty::FileProperty(const std::string & name, const std::string& default_value, unsigned int action,
         const std::string & ext, unsigned int direction)
  : PropertyWithValue<std::string>(name, default_value,
      /* Create either a FileValidator or a DirectoryValidator, depending on Action */
      (action == FileProperty::Directory || action == FileProperty::OptionalDirectory) ?
          new DirectoryValidator(action == FileProperty::Directory) :
          new FileValidator(std::vector<std::string>(1,ext), (action == FileProperty::Load) )
      , direction),
    m_action(action),
    m_defaultExt(ext),
    m_runFileProp(false)
{
  setUp(ext);
}



/**
 * Check if this is a load property
 * @returns True if the property is a Load property and false otherwise
 */
bool FileProperty::isLoadProperty() const
{
  return m_action == Load || m_action == OptionalLoad;
}

/**
 * Check if this is a Save property
 * @returns True if the property is a Save property and false otherwise
 */
bool FileProperty::isSaveProperty() const
{
  return m_action == Save || m_action == OptionalSave;
}


/**
 * Check if this is a directory selection property
 * @returns True if the property is a Directory property
 */
bool FileProperty::isDirectoryProperty() const
{
  return m_action == Directory || m_action == OptionalDirectory;
}

/**
* Check if this property is optional
* @returns True if the property is optinal, false otherwise
*/
bool FileProperty::isOptional() const
{
  return (m_action == OptionalLoad || m_action == OptionalSave);
}

/**
 * Set the value of the property
 * @param propValue :: The value here is treated as relating to a filename
 * @returns A string indicating the outcome of the attempt to set the property. An empty string indicates success.
 */
std::string FileProperty::setValue(const std::string & propValue)
{
  // Empty value is allowed if optional
  if( propValue.empty() )
  {
    PropertyWithValue<std::string>::setValue("");
    if( isOptional() )
    {
      return "";
    }
    else
    { 
      return "No file specified.";
    }
  }

  // If this looks like an absolute path then don't do any searching but make sure the 
  // directory exists for a Save property
  if( Poco::Path(propValue).isAbsolute() )
  {
    std::string error("");
    if( isSaveProperty() )
    {
       error = createDirectory(propValue);
       if( !error.empty() ) return error;
    }

    return PropertyWithValue<std::string>::setValue(propValue);
  }

  std::string errorMsg("");
  // For relative paths, differentiate between load and save types
  if( isLoadProperty() )
  {
    errorMsg = setLoadProperty(propValue);
  }
  else
  {
    errorMsg = setSaveProperty(propValue);
  }
  return errorMsg;
}

/**
 * Set up the property
 * @param defExt :: The default extension
 */
void FileProperty::setUp(const std::string & defExt)
{
  m_defaultExt = defExt;
  if( isLoadProperty() && extsMatchRunFiles() )
  {
    m_runFileProp = true;
  }
  else
  {
    m_runFileProp = false;
  }
}

/**
 * Do the allowed values match the facility preference extensions for run files
 * @returns True if the extensions match those in the facility's preference list for 
 * run file extensions, false otherwise
 */
bool FileProperty::extsMatchRunFiles()
{
  Kernel::FacilityInfo facilityInfo = Kernel::ConfigService::Instance().getFacility();
  const std::vector<std::string>  facilityExts = facilityInfo.extensions();
  std::vector<std::string>::const_iterator facilityExtsBegin = facilityExts.begin();
  std::vector<std::string>::const_iterator facilityExtsEnd = facilityExts.end();
  const std::set<std::string> allowedExts = this->allowedValues();

  bool match(false);
  for( std::set<std::string>::const_iterator it = allowedExts.begin(); it != allowedExts.end(); ++it )
  {
    if( std::find(facilityExtsBegin, facilityExtsEnd, *it) != facilityExtsEnd )
    {
      match = true;
      break;
    }
  }

  return match;
}

namespace { // anonymous namespace keeps it here
void addExtension(const std::string &extension, std::vector<std::string> &extensions)
{
  if (std::find(extensions.begin(), extensions.end(), extension) != extensions.end())
    return;
  else
    extensions.push_back(extension);
}
}

/**
 * Handles the filename if this is a load property
 * @param propValue :: The filename to treat as a filepath to be loaded
 * @returns A string contain the result of the operation, empty if successful.
 */
std::string FileProperty::setLoadProperty(const std::string & propValue)
{
  std::string foundFile("");
  if( m_runFileProp )
  {
    std::set<std::string> allowedExts(allowedValues());
    std::vector<std::string> exts;
    if (!m_defaultExt.empty())
    {
      addExtension(m_defaultExt, exts);

      std::string lower(m_defaultExt);
      std::transform(m_defaultExt.begin(), m_defaultExt.end(), lower.begin(), tolower);
      addExtension(lower, exts);

      std::string upper(m_defaultExt);
      std::transform(m_defaultExt.begin(), m_defaultExt.end(), upper.begin(), toupper);
      addExtension(upper, exts);
    }
    for(std::set<std::string>::iterator it = allowedExts.begin();it!=allowedExts.end();++it)
    {
      std::string lower(*it); 
      std::string upper(*it); 
      std::transform(it->begin(), it->end(), lower.begin(), tolower);
      std::transform(it->begin(), it->end(), upper.begin(), toupper);
      addExtension(*it, exts);
      addExtension(lower, exts);
      addExtension(upper, exts);
    }
    foundFile = FileFinder::Instance().findRun(propValue, exts);
  }
  else
  {
    foundFile = FileFinder::Instance().getFullPath(propValue);
  }

  if( foundFile.empty() )
  {
    return PropertyWithValue<std::string>::setValue(propValue);
  }
  else
  {
    return PropertyWithValue<std::string>::setValue(foundFile);
  }
}

/**
 * Handles the filename if this is a save property
 * @param propValue :: The filename to treat as a filepath to be saved
 * @returns A string contain the result of the operation, empty if successful.
 */
std::string FileProperty::setSaveProperty(const std::string & propValue)
{
  if( propValue.empty() )
  {
    if ( m_action == OptionalSave )
    {
      return PropertyWithValue<std::string>::setValue("");
    }
    else return "Empty filename not allowed.";
  }
  std::string errorMsg("");
  // We have a relative save path so just prepend the path that is in the 'defaultsave.directory'
  // Note that this catches the Poco::NotFoundException and returns an empty string in that case
  std::string save_path =  ConfigService::Instance().getString("defaultsave.directory");
  Poco::Path save_dir;
  if( save_path.empty() )
  {
    save_dir = Poco::Path(propValue).parent();
    // If we only have a stem filename, parent() will make save_dir empty and then Poco::File throws
    if( save_dir.toString().empty() )
    {
      save_dir = Poco::Path::current();
    }
  }
  else
  {
    save_dir = Poco::Path(save_path).makeDirectory();
  }
  errorMsg = createDirectory(save_dir.toString());
  if( errorMsg.empty() )
  {
    std::string fullpath = save_dir.resolve(propValue).toString();
    errorMsg = PropertyWithValue<std::string>::setValue(fullpath);
  }
  return errorMsg;
}

/**
 * Create a given directory if it does not already exist.
 * @param path :: The path to the directory, which can include file stem
 * @returns A string indicating a problem if one occurred
 */
std::string FileProperty::createDirectory(const std::string & path) const
{
  Poco::Path stempath(path);
  if( stempath.isFile() )
  {
    stempath.makeParent();
  }
  std::string error("");
  if( !stempath.toString().empty() )
  {
    Poco::File stem(stempath);
    if( !stem.exists() )
    {
      try
      {
	stem.createDirectories();
      }
      catch(Poco::Exception &e)
      {
	error = e.what();
      }
    }
  }
  else
  {
    error = "Invalid directory.";
  }
  return error;
}

/**
 * Check file extension to see if a lower- or upper-cased version will also match if the given one does not exist
 * @param filepath :: A filename whose extension is checked and converted to lower/upper case if necessary.
 * @returns The new filename
 */
std::string FileProperty::convertExtension(const std::string & filepath) const
{
  Poco::Path fullpath(filepath);
  std::string ext = fullpath.getExtension();
  if( ext.empty() ) return filepath;
  const size_t nchars = ext.size();
  for( size_t i = 0; i < nchars; ++i )
  {
    int c = static_cast<int>(ext[i]);
    if( std::islower(c) )
    {
      ext[i] = static_cast<char>(std::toupper(c));
    }
    else if( std::isupper(c) )
    {
      ext[i] = static_cast<char>(std::tolower(c));
    }
    else {}
  }
  fullpath.setExtension(ext);
  return fullpath.toString();  
}

}
}
