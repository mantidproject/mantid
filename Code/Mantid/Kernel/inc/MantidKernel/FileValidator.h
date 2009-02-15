#ifndef MANTID_KERNEL_FILEVALIDATOR_H_
#define MANTID_KERNEL_FILEVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "IValidator.h"
#include <set>
#include <algorithm>
#include "Poco/File.h"

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
  FileValidator() : IValidator<std::string>(), m_extensions(), m_fullTest(true)
  {}

  /** Constructor
   *  @param extensions The permitted file extensions (e.g. .RAW)
   *  @param testFileExists Flag indicating whether to test for existence of file (default: yes)
   */
  explicit FileValidator(const std::vector<std::string>& extensions, bool testFileExists = true) :
    IValidator<std::string>(),
    m_extensions(extensions),
    m_fullTest(testFileExists)
  {}

  /// Destructor
  virtual ~FileValidator() {}

  /** Checks whether the value provided is a valid filepath.
   *  @param value The value to test
   *  @return True if the value is valid, false otherwise
   */
  const bool isValid(const std::string &value) const
  {
    if (m_extensions.size() > 0)
    {
      //Find extension of value
      std::size_t found=value.find_last_of(".");
      std::string ext = value.substr(found+1);

      std::vector<std::string>::const_iterator itr;

      itr = std::find(m_extensions.begin(), m_extensions.end(), ext);

      if (itr == m_extensions.end()) return false;
    }

    if ( m_fullTest && !value.empty() && !Poco::File(value).exists() )
    {
      return false;
    }

    return true;
  }

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

  IValidator<std::string>* clone() { return new FileValidator(*this); }

private:
  /// The list of permitted extensions
  const std::vector<std::string> m_extensions;
  /// Flag indicating whether to test for existence of filename
  const bool m_fullTest;

};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_FILEVALIDATOR_H_*/
