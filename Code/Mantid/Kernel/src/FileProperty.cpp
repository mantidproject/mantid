//-----------------------------------------------------------------
// Includes
//-----------------------------------------------------------------
#include "MantidKernel/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "Poco/Path.h"

using namespace Mantid::Kernel;

//-----------------------------------------------------------------
// Public member functions
//-----------------------------------------------------------------
/**
 * Constructor
 * @param name The name of the property
 * @param default_value A default value for the property
 * @param exts A vector containing the allowed extensions
 * @param action An enum indicating whether this should be a load/save property
 * @param direction An optional direction (default=Input)
 */
FileProperty::FileProperty(const std::string & name, const std::string& default_value, unsigned int action,
			   const std::vector<std::string> & exts, unsigned int direction) 
  : PropertyWithValue<std::string>(name, default_value, new FileValidator(exts, (action > FileProperty::NoExistLoad) ), 
				   direction), 
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
  // If the path is absolute then don't do any searching
  if( filename.empty() || Poco::Path(filename).isAbsolute())
  {
    return PropertyWithValue<std::string>::setValue(filename);
  }

  std::string valid_string("");
  // For relative paths, differentiate between load and save types
  if( m_action == FileProperty::Load )
  {
    Poco::File relative(filename);
    Poco::File check_file(Poco::Path(Poco::Path::current()).resolve(relative.path()));
    //Do a quick check relative to the current directory first
    if( check_file.exists() )
    {
      valid_string = PropertyWithValue<std::string>::setValue(check_file.path());
    }
    else
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
    // We have a relative save path so just prepend the path that is in the 'defaultsave.directory'
    std::string save_path =  ConfigService::Instance().getString("defaultsave.directory");
    Poco::Path save_dir;
    if( save_path.empty() )
    {
      save_dir = Poco::Path(filename).parent();
    }
    else
    {
      save_dir = Poco::Path(save_path).makeDirectory();
    }
    if( Poco::File(save_dir).canWrite() )
    {
      valid_string = PropertyWithValue<std::string>::setValue(save_dir.resolve(filename).toString());
    }
    else
    {
      valid_string = "Cannot write file to path \"" + save_dir.toString() + "\". Location is not writable.";
    }
  }
  return valid_string;
}
