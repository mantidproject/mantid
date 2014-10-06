#include "MantidKernel/FileValidator.h"
#include "MantidKernel/Logger.h"
#include <algorithm>
#include <boost/algorithm/string/case_conv.hpp>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <iostream>

namespace Mantid
{
namespace Kernel
{

namespace
{
  // Initialize the static logger
  Logger g_log("FileValidator");
}

/** Constructor
 *  @param extensions :: The permitted file extensions (e.g. .RAW)
 *  @param testFileExists :: Flag indicating whether to test for existence of file (default: yes)
 *  @param testCanWrite :: Flag to check if file writing permissible.
 */
FileValidator::FileValidator(const std::vector<std::string>& extensions, bool testFileExists,
                             bool testCanWrite) :
  TypedValidator<std::string>(),
  m_testExist(testFileExists),
  m_testCanWrite(testCanWrite)
{
  for(auto it = extensions.begin(); it != extensions.end(); ++it)
  {
    const std::string ext = boost::to_lower_copy(*it);
    if ( std::find(m_extensions.begin(), m_extensions.end(), ext) == m_extensions.end() )
    {
      m_extensions.push_back( ext );
    }
  }
}

/// Destructor
FileValidator::~FileValidator() {}

/// Returns the set of valid values
std::vector<std::string> FileValidator::allowedValues() const
{
  return m_extensions;
}

/** 
 * Clone the validator
 * @returns A pointer to a new validator with the same properties as this one
 */
IValidator_sptr FileValidator::clone() const
{ 
  return boost::make_shared<FileValidator>(*this); 
}

/** If m_fullTest=true if checks that the files exists, otherwise just that path syntax looks valid
 *  @param value :: file name
 *  @returns An error message to display to users or an empty string on no error
 */
std::string FileValidator::checkValidity(const std::string &value) const
{
  // Check if the path is syntactically valid
  if( !Poco::Path().tryParse(value) )
  {
    return "Error in path syntax: \"" + value + "\".";
  }
  
  //Check the extension but just issue a warning if it is not one of the suggested values
  if (!(value.empty()))
  {
    if (!(this->endswith(value)))
    {
      //Dropped from warning to debug level as it was printing out on every search of the archive, even when successful. re #5998
      g_log.debug() << "Unrecognised extension in file \"" << value << "\"";
      if (!this->m_extensions.empty()) {
        g_log.debug() << " [ ";
        for (auto it = this->m_extensions.begin(); it != this->m_extensions.end(); ++it)
          g_log.debug() << *it << " ";
        g_log.debug() << "]";
      }
      g_log.debug() << "\"."  << std::endl;
    }
  }

  // create a variable for the absolute path to be used in error messages
  std::string abspath(value);
  if (!value.empty())
  {
    Poco::Path path(value);
    if (path.isAbsolute())
      abspath = path.toString();
  }

  //If the file is required to exist check it is there
  if ( m_testExist && ( value.empty() || !Poco::File(value).exists() ) )
  {
    return "File \"" + abspath + "\" not found";
  }

  //If the file is required to be writable...
  if (m_testCanWrite)
  {
    if (value.empty())
      return "Cannot write to empty filename";

    Poco::File file(value);
    // the check for writable is different for whether or not a version exists
    // this is taken from ConfigService near line 443
    if (file.exists())
    {
      try
      {
        if (!file.canWrite())
          return "File \"" + abspath + "\" cannot be written";
      }
      catch (std::exception &e)
      {
        g_log.information() << "Encountered exception while checking for writable: " << e.what();
      }
    }
    else // if the file doesn't exist try to temporarily create one
    {
      try
      {
        Poco::Path direc(value);
        if (direc.isAbsolute())
        {
          // see if file is writable
          if (Poco::File(direc).canWrite())
            return "";
          else
            return "Cannot write to file \"" + direc.toString() + "\"";
        }

        g_log.debug() << "Do not have enough information to validate \""
                      << abspath << "\"\n";
      }
      catch (std::exception &e)
      {
        g_log.information() << "Encountered exception while checking for writable: " << e.what();
      }
      catch (...) {
    	g_log.information() << "Unknown exception while checking for writable";
      }
    }
  }

  //Otherwise we are okay, file extensions are just a suggestion so no validation on them is necessary
  return "";
}

/**
 * Confirm that the value string ends with then ending string.
 * @param value :: The string to check the ending for.
 * @param ending :: The ending the string should have.
 */
bool has_ending(const std::string &value, const std::string & ending)
{
  if (ending.empty()) // always match against an empty extension
    return true;
  if (value.length() < ending.length()) // filename is not long enough
    return false;
  int result = value.compare(value.length() - ending.length(), ending.length(), ending);
  return (result == 0); // only care if it matches
}

/**
 * Checks the extension of a filename
 * @param value :: the filename to check
 * @return flag that true if the extension matches in the filename
 */
bool FileValidator::endswith(const std::string &value) const
{
  if (m_extensions.empty()) // automatically match a lack of extensions
    return true;
  if ((m_extensions.size() == 1) && (m_extensions.begin()->empty()))
    return true;

  // create a lowercase copy of the filename
  std::string value_copy(value);
  std::transform(value_copy.begin(), value_copy.end(), value_copy.begin(), tolower);

  // check for the ending
  for (auto it = m_extensions.begin();
       it != m_extensions.end(); ++it) {
    if (has_ending(value, *it)) // original case
      return true;
    if (has_ending(value_copy, *it)) // lower case
      return true;
  }
  return false;
}

} // namespace Kernel
} // namespace Mantid
