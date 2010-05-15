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

/// Initialize the logger
Logger& FileValidator::g_log = Logger::get("FileValidator");

/// Default constructor.
FileValidator::FileValidator() : IValidator<std::string>(), m_extensions(), m_fullTest(true)
{}

/** Constructor
 *  @param extensions The permitted file extensions (e.g. .RAW)
 *  @param testFileExists Flag indicating whether to test for existence of file (default: yes)
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
 *  @param value file name
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
  std::string::size_type idx = value.rfind(".");
  if( idx != std::string::npos )
  {
    std::string ext = value.substr(idx + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
    if( !m_extensions.empty() && m_extensions.find(ext) == m_extensions.end() )
    {
      g_log.warning() << "Unrecognised extension in file \"" << value  << "\"."  << std::endl;
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


} // namespace Kernel
} // namespace Mantid
