#include "MantidKernel/FileValidator.h"
#include <algorithm>
#include "Poco/File.h"
#include "Poco/Path.h"
#include <iostream>


namespace
{
  /// Functor object to supply to for_each
  struct lowercase
  {
    void operator()(std::string s)
    {
      std::transform(s.begin(), s.end(), s.begin(), tolower);
    }
  };
}

namespace Mantid
{
namespace Kernel
{

// Initialize the logger
Logger& FileValidator::g_log = Logger::get("FileValidator");

/// Default constructor.
FileValidator::FileValidator() : IValidator<std::string>(), m_extensions(), m_fullTest(true)
{}

/** Constructor
 *  @param extensions :: The permitted file extensions (e.g. .RAW)
 *  @param testFileExists :: Flag indicating whether to test for existence of file (default: yes)
 */
FileValidator::FileValidator(const std::vector<std::string>& extensions, bool testFileExists) :
  IValidator<std::string>(),
  m_extensions(extensions.begin(),extensions.end()),
  m_fullTest(testFileExists)
{
  for_each(m_extensions.begin(), m_extensions.end(), lowercase());
}

/// Destructor
FileValidator::~FileValidator() {}

/// Returns the set of valid values
std::set<std::string> FileValidator::allowedValues() const
{
  return m_extensions;
}

/** 
 * Clone the validator
 * @returns A pointer to a new validator with the same properties as this one
 */
IValidator<std::string>* FileValidator::clone() 
{ 
  return new FileValidator(*this); 
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
      g_log.warning() << "Unrecognised extension in file \"" << value << "\"";
      if (!this->m_extensions.empty()) {
        this->g_log.warning() << " [ ";
        for (std::set<std::string>::const_iterator it = this->m_extensions.begin(); it != this->m_extensions.end(); ++it)
          g_log.warning() << *it << " ";
        this->g_log.warning() << "]";
      }
      g_log.warning() << "\"."  << std::endl;
    }
  }

  //If the file is required to exist check it is there
  if ( m_fullTest && ( value.empty() || !Poco::File(value).exists() ) )
  {
    return "File \"" + Poco::Path(value).getFileName() + "\" not found";
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
  for (std::set<std::string>::const_iterator it = m_extensions.begin();
       it != m_extensions.end(); it++) {
    if (has_ending(value, *it)) // original case
      return true;
    if (has_ending(value_copy, *it)) // lower case
      return true;
  }
  return false;
}

} // namespace Kernel
} // namespace Mantid
