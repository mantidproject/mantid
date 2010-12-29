#ifndef MANTID_KERNEL_DirectoryValidator_H_
#define MANTID_KERNEL_DirectoryValidator_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "IValidator.h"
#include "MantidKernel/FileValidator.h"
#include <vector>

namespace Mantid
{
namespace Kernel
{


/** DirectoryValidator is a validator that checks that a directory path is valid.

    @author Janik Zikovsky, SNS
    @date Nov 12, 2010

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport DirectoryValidator : public FileValidator
{
public:
  explicit DirectoryValidator(bool testDirectoryExists = true);
  virtual ~DirectoryValidator();
  virtual std::set<std::string> allowedValues() const;
  virtual IValidator<std::string>* clone();

private:
  virtual std::string checkValidity(const std::string &value) const;

  /// A reference to the logger
  static Logger & g_log;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_DirectoryValidator_H_*/
