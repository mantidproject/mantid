#ifndef MANTID_KERNEL_FILEVALIDATOR_H_
#define MANTID_KERNEL_FILEVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "IValidator.h"
#include <set>
#include <algorithm>
#include "Poco/File.h"
#include "Poco/RegularExpression.h"

/// A functor for checking the file extensions against a regular expression
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

  ///Converts a shell-like pattern to a regular expression pattern
  struct RegExConverter
  {
    /// Operator to convert from shell-like patterns to regular expression syntax
    std::string operator()(const std::string & pattern)
    {
      std::string replacement;
      std::string::const_iterator iend = pattern.end();
      for( std::string::const_iterator itr = pattern.begin(); itr != iend; ++itr )
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

/**
 * Mantid namespace
 */
namespace Mantid
{
namespace Kernel
{
/** FileValidator is a validator that checks that a filepath is valid.

    @author Matt Clarke, ISIS.
    @date 25/06/2008

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport FileValidator : public IValidator<std::string>
{
public:
  /// Default constructor.
  FileValidator() : IValidator<std::string>(), m_extensions(), m_regex_exts(), m_fullTest(true)
  {}

  /** Constructor
   *  @param extensions The permitted file extensions (e.g. .RAW)
   *  @param testFileExists Flag indicating whether to test for existence of file (default: yes)
   */
  explicit FileValidator(const std::vector<std::string>& extensions, bool testFileExists = true) :
    IValidator<std::string>(),
    m_extensions(extensions),
    m_regex_exts(m_extensions.size()),
    m_fullTest(testFileExists)
  {
    // Transform the file extensions to regular expression syntax for matching. This could be done every time
    // isValid is called but that would create unnecessary work
    std::transform( m_extensions.begin(), m_extensions.end(), m_regex_exts.begin(), RegExConverter() );
  }

  /// Destructor
  virtual ~FileValidator() {}

   ///Return the type of the validator
  const std::string getType() const
  {
    return "file";
  }

  /// Returns the set of valid values
  const std::vector<std::string>& allowedValues() const
  {
    return m_extensions;
  }

  /** 
   * Is this validator concerned with whether the file exists or not
   * @returns A boolean indicating whether this tests for file existence
   */
  bool fileMustExist() const
  {
    return m_fullTest;
  }

  /** 
   * Clone the validator
   * @returns A pointer to a new validator with the same properties as this one
   */
  IValidator<std::string>* clone() 
  { 
    return new FileValidator(*this); 
  }
  ///The maximum number of file extensions to report in error messages defined to stop error messages from becoming too long
  static const int MAX_NUM_EXTENSIONS = 20;
private:
  /// The list of permitted extensions
  const std::vector<std::string> m_extensions;
  /// An internal list of extensions that, if necessary, have been transformed into regular expressions
  std::vector<std::string> m_regex_exts; 
  /// Flag indicating whether to test for existence of filename
  const bool m_fullTest;

  /// Returns A user level error if the name has the wrong extension and if m_fullTest=true the named file doesn't exist
  std::string FileValidator::checkValidity(const std::string &value) const;

};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_FILEVALIDATOR_H_*/
