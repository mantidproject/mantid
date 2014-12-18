#ifndef MANTID_KERNEL_FILEVALIDATOR_H_
#define MANTID_KERNEL_FILEVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/TypedValidator.h"
#include <vector>

namespace Mantid
{
namespace Kernel
{

bool has_ending(const std::string &value, const std::string & ending);

/** FileValidator is a validator that checks that a filepath is valid.

    @author Matt Clarke, ISIS.
    @date 25/06/2008

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_KERNEL_DLL FileValidator : public TypedValidator<std::string>
{
public:
  explicit FileValidator(const std::vector<std::string>& extensions=std::vector<std::string>(), bool testFileExists = true, bool testCanWrite=false);
  virtual ~FileValidator();
  virtual std::vector<std::string> allowedValues() const;
  IValidator_sptr clone() const;

protected:
  /// The list of permitted extensions
  std::vector<std::string> m_extensions;
  /// Flag indicating whether to test for existence of filename
  bool m_testExist;
  /// Flag indicating whether to test for the file being writable
  bool m_testCanWrite;
  
private:
  virtual std::string checkValidity(const std::string &value) const;
  bool endswith(const std::string &value) const;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_FILEVALIDATOR_H_*/
