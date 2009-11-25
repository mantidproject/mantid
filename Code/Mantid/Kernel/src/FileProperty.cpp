//-----------------------------------------------------------------
// Includes
//-----------------------------------------------------------------
#include "MantidKernel/FileProperty.h"
#include "MantidKernel/FileValidator.h"
#include "MantidKernel/ConfigService.h"
#include "Poco/Path.h"
#include "Poco/File.h"

using namespace Mantid::Kernel;

//-----------------------------------------------------------------
// Public member functions
//-----------------------------------------------------------------
/**
 * Constructor
 * @param name          The name of the property
 * @param default_value A default value for the property
 * @param exts          The set of allowed extensions
 * @param action        An enum indicating whether this should be a load/save property
 * @param direction     An optional direction (default=Input)
 */
FileProperty::FileProperty(const std::string & name, const std::string& default_value, unsigned int action,
			   const std::vector<std::string> & exts, unsigned int direction) 
  : PropertyWithValue<std::string>(name, default_value, new FileValidator(exts, (action > FileProperty::NoExistLoad) ), direction), 
    m_action(action)
{
}

/**
 * Check if this is a load property
 * @returns True if the property is a Load property and false if a Save type
 */
bool FileProperty::isLoadProperty() const
{
  return (m_action != FileProperty::Save);
}

/**
 * Set the filename
 * @param filename The value here is treated as a filename.
 * @returns A string indicating the outcome of the attempt to set the property. An empty string indicates success.
 */
std::string FileProperty::setValue(const std::string & filename)
{
  // If the path is absolute then don't do any searching but make sure the directory exists for a Save property
  if( Poco::Path(filename).isAbsolute() )
  {
    if( !isLoadProperty() )
    {
      checkDirectory(filename);
    }
    return PropertyWithValue<std::string>::setValue(filename);
  }

  std::string valid_string("");
  // For relative paths, differentiate between load and save types
  if( isLoadProperty() )
  {
    if( filename.empty() ) return PropertyWithValue<std::string>::setValue(filename);

    Poco::File relative(filename);
    Poco::File check_file(Poco::Path(Poco::Path::current()).resolve(relative.path()));
    valid_string = PropertyWithValue<std::string>::setValue(check_file.path());
    if( !valid_string.empty() )
    {
      const std::vector<std::string>& search_dirs = ConfigService::Instance().getDataSearchDirs();
      std::vector<std::string>::const_iterator iend = search_dirs.end();
      for( std::vector<std::string>::const_iterator it = search_dirs.begin(); it != iend; ++it )
      {
        check_file = Poco::File(Poco::Path(*it).resolve(relative.path()));
        if( check_file.exists() )
        {
          valid_string = PropertyWithValue<std::string>::setValue(check_file.path());
          break;
        }
      }
    }
  }
  else
  {
    if( filename.empty() ) return "Empty filename not allowed.";

    // We have a relative save path so just prepend the path that is in the 'defaultsave.directory'
    // Note that this catches the Poco::NotFoundException and returns an empty string in that case
    std::string save_path =  ConfigService::Instance().getString("defaultsave.directory");
    Poco::Path save_dir;
    if( save_path.empty() )
    {
      save_dir = Poco::Path(filename).parent();
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
    if( Poco::File(save_dir).canWrite() )
    {
      std::string fullpath = save_dir.resolve(filename).toString();
      checkDirectory(fullpath);
      valid_string = PropertyWithValue<std::string>::setValue(fullpath);
    }
    else
    {
      valid_string = "Cannot write file to path \"" + save_dir.toString() + "\". Location is not writable.";
    }
  }
  return valid_string;
}

/**
 * Check whether a given directory exists and create it if it does not.
 * @param fullpath The path to the directory, which can include file stem
 */
void FileProperty::checkDirectory(const std::string & fullpath) const
{
  Poco::Path stempath(fullpath);
  if( stempath.isFile() )
  {
    stempath.makeParent();
  }
  if( !stempath.toString().empty() )
  {
    Poco::File stem(stempath);
    if( !stem.exists() )
    {
      stem.createDirectories();
    }
  }
}
