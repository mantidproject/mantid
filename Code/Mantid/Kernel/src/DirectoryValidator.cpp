#include "MantidKernel/DirectoryValidator.h"
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
Logger& DirectoryValidator::g_log = Logger::get("DirectoryValidator");

/** Constructor
 *  @param testDirectoryExists Flag indicating whether to test for existence of directory (default: yes)
 */
DirectoryValidator::DirectoryValidator(bool testDirectoryExists)
  : FileValidator()
{
  this->m_fullTest = testDirectoryExists;
}

/// Destructor
DirectoryValidator::~DirectoryValidator() {}

/// Returns the set of valid values
std::set<std::string> DirectoryValidator::allowedValues() const
{
  return std::set<std::string>();
}

/** 
 * Clone the validator
 * @returns A pointer to a new validator with the same properties as this one
 */
IValidator<std::string>* DirectoryValidator::clone()
{ 
  return new DirectoryValidator(*this);
}

/** If m_fullTest=true if checks that the files exists, otherwise just that path syntax looks valid
 *  @param value file name
 *  @returns An error message to display to users or an empty string on no error
 */
std::string DirectoryValidator::checkValidity(const std::string &value) const
{
  // Check if the path is syntactically valid
  if( !Poco::Path().tryParse(value) )
  {
    return "Error in path syntax: \"" + value + "\".";
  }

  //If the path is required to exist check it is there
  if ( m_fullTest )
  {
   if ( value.empty() || !Poco::File(value).exists() )
    return "Directory \"" + value + "\" not found";
   if (!Poco::File(value).isDirectory())
     return "Directory \"" + value + "\" specified is actually a file";
  }

  //Otherwise we are okay, file extensions are just a suggestion so no validation on them is necessary
  return "";
}


} // namespace Kernel
} // namespace Mantid
