#include "MantidKernel/FileValidator.h"
#include <algorithm>
#include "Poco/File.h"
#include "Poco/RegularExpression.h"
#include <iostream>

/// Functors for checking the file extensions against a regular expression
namespace {
  /// A struct holding a string to be tested against a regular expression
  struct RegExMatcher
  {
    /// Constructor
    RegExMatcher(std::string ext) : m_ext(ext) {}
  
    /// Operator matches expressions
    bool operator()(const std::string & test)
    {
      Poco::RegularExpression regex(test, Poco::RegularExpression::RE_CASELESS);
      bool matched = regex.match(m_ext);
      return matched;
    }
  
  private:
    /// Private default constructor
    RegExMatcher();
    /// The extension to test
    std::string m_ext;
  };

  /// Converts a shell-like pattern to a regular expression pattern
  struct RegExConverter
  {
    /// Operator to convert from shell-like patterns to regular expression syntax
    std::string operator()(const std::string & pattern)
    {
      std::string replacement;
      for( std::string::const_iterator itr = pattern.begin(); itr != pattern.end(); ++itr )
      {
        char ch = *itr;
        if( ch == '?' ) replacement.append(".");
        else if( ch == '*' ) replacement.append(".*");
        else replacement.push_back(ch);
      }
      return replacement;
    }
  };  
}

namespace Mantid
{
namespace Kernel
{
/// Default constructor.
FileValidator::FileValidator() : IValidator<std::string>(), m_extensions(), m_regex_exts(), m_fullTest(true)
{}

/** Constructor
 *  @param extensions The permitted file extensions (e.g. .RAW)
 *  @param testFileExists Flag indicating whether to test for existence of file (default: yes)
 */
FileValidator::FileValidator(const std::vector<std::string>& extensions, bool testFileExists) :
  IValidator<std::string>(),
  m_extensions(extensions.begin(),extensions.end()),
  m_regex_exts(),
  m_fullTest(testFileExists)
{
  // Transform the file extensions to regular expression syntax for matching. This could be done every time
  // checkValidity() is called but that would create unnecessary work
  RegExConverter conv;
  std::set<std::string>::const_iterator it;
  for (it = m_extensions.begin(); it != m_extensions.end(); ++it)
  {
    m_regex_exts.insert(conv(*it));
  }
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

/** If m_fullTest=true if checks that the files exists, otherwise just that the extension is good 
 *  @param value file name
 *  @returns An error message to display to users or an empty string on no error
 */
std::string FileValidator::checkValidity(const std::string &value) const
{
  //check the file has a good extension
  if( !m_regex_exts.empty() )
  {
    //Find extension of value
    const std::string ext = value.substr(value.rfind(".") + 1);
    //Use a functor to test each allowed extension in turn
    RegExMatcher matcher(ext); 
    std::set<std::string>::const_iterator itr = 
          std::find_if(m_regex_exts.begin(), m_regex_exts.end(), matcher);

    // If not found, return the valid extensions
    if ( itr == m_regex_exts.end() )
    {
      if (m_extensions.size() == 1)
      {
        return "The file must have extension " + *(m_extensions.begin());
      }
      else
      {
        std::string error("The file must have one of these extensions: ");
        std::set<std::string>::const_iterator itr = m_extensions.begin();
        error += *itr;
        for ( ++itr; itr != m_extensions.end(); ++itr )
        {
          error += ", " + *itr;
        }
        return error;        
      }
    }
  }
    
  //If the file is required to exist check it is there
  if ( m_fullTest && ( value.empty() || !Poco::File(value).exists() ) )
  {
    return "File \"" + value + "\" not found";
  }
  return "";
}


} // namespace Kernel
} // namespace Mantid
