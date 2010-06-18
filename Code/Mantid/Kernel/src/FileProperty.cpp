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
  : PropertyWithValue<std::string>(name, default_value, new FileValidator(exts, (action == FileProperty::Load) ), direction), 
    m_action(action)
{
}

/**
 * Constructor
 * @param name          The name of the property
 * @param default_value A default value for the property
 * @param ext           The allowed extension
 * @param action        An enum indicating whether this should be a load/save property
 * @param direction     An optional direction (default=Input)
 */
FileProperty::FileProperty(const std::string & name, const std::string& default_value, unsigned int action,
         const std::string & ext, unsigned int direction)
  : PropertyWithValue<std::string>(name, default_value,
           new FileValidator(std::vector<std::string>(1,ext), (action == FileProperty::Load) ), direction),
    m_action(action)
{
}

/**
 * Check if this is a load property
 * @returns True if the property is a Load property and false if a Save type
 */
bool FileProperty::isLoadProperty() const
{
  return m_action == Load || m_action == OptionalLoad;
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
    std::string error("");
    if( !isLoadProperty() )
    {
       error = checkDirectory(filename);
       if( !error.empty() ) return error;
    }

    error = PropertyWithValue<std::string>::setValue(filename);
    if( error.empty() ) return error;
    // Change the file extension to a lower/upper cased version of the extension to check if this can be found instead
    std::string diffcase_ext = convertExtension(filename);
    return PropertyWithValue<std::string>::setValue(diffcase_ext);
  }

  std::string valid_string("");
  // For relative paths, differentiate between load and save types
  if( isLoadProperty() )
  {
    if( filename.empty() ) return PropertyWithValue<std::string>::setValue(filename);

    Poco::File relative(filename);
    Poco::File check_file(Poco::Path(Poco::Path::current()).resolve(relative.path()));
    valid_string = PropertyWithValue<std::string>::setValue(check_file.path());
    if( !valid_string.empty() || !check_file.exists() )
    {
      std::string diffcase_ext = convertExtension(filename);
      valid_string = PropertyWithValue<std::string>::setValue(diffcase_ext);
      check_file = Poco::Path(Poco::Path::current()).resolve(diffcase_ext);
      if( valid_string.empty() && check_file.exists() ) return "";

      Poco::File relative_diffext(diffcase_ext);
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
        check_file = Poco::File(Poco::Path(*it).resolve(relative_diffext.path()));
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
    if( filename.empty() )
    {
      if ( m_action == OptionalSave )
      {
        return PropertyWithValue<std::string>::setValue("");
      }
      else return "Empty filename not allowed.";
    }

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
    valid_string = checkDirectory(save_dir.toString());
    if( valid_string.empty() )
    {
      std::string fullpath = save_dir.resolve(filename).toString();
      valid_string = PropertyWithValue<std::string>::setValue(fullpath);
    }

  }
  return valid_string;
}

/**
 * Check whether a given directory exists and create it if it does not.
 * @param fullpath The path to the directory, which can include file stem
 * @returns A string indicating a problem if one occurred
 */
std::string FileProperty::checkDirectory(const std::string & fullpath) const
{
  Poco::Path stempath(fullpath);
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
 * @param filepath A filename whose extension is checked and converted to lower/upper case if necessary.
 * @returns The new filename
 */
std::string FileProperty::convertExtension(const std::string & filepath) const
{
  Poco::Path fullpath(filepath);
  std::string ext = fullpath.getExtension();
  if( ext.empty() ) return "";
  int nchars = ext.size();
  for( int i = 0; i < nchars; ++i )
  {
    int c = static_cast<int>(ext[i]);
    if( c >= 65 && c <= 90 )
    {
      ext[i] = static_cast<char>(c + 32);
    }
    else if( c >= 97 && c <= 122 )
    {
      ext[i] = static_cast<char>(c - 32);
    }
    else
    {
    }
  }
  fullpath.setExtension(ext);
  return fullpath.toString();  
}
