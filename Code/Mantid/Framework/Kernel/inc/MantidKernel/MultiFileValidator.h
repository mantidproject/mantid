#ifndef MANTID_KERNEL_MULTIFILEVALIDATOR_H_
#define MANTID_KERNEL_MULTIFILEVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "FileValidator.h"

#include <vector>
#include <set>
#include <string>

namespace Mantid
{
namespace Kernel
{
/** The MultiFileValidator validates a MultiFileProperty, which contains a *vector of
    vectors* of filenames - the meaning of which is discussed in MultiFileProperty.h.

    This is essentially a wrapper around the FileValidator class; a single instance
    of which is called, once for each filename.

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class MANTID_KERNEL_DLL MultiFileValidator : public TypedValidator<std::vector<std::vector<std::string> > >
{
public:
  MultiFileValidator();
  MultiFileValidator(const MultiFileValidator & mfv);
  explicit MultiFileValidator(const std::vector<std::string>& extensions);
  virtual ~MultiFileValidator();

  IValidator_sptr clone() const;

  /// Returns the set of allowed extensions.
  virtual std::vector<std::string> allowedValues() const;

protected:
  /// FileValidator instance used for validating multiple files.
  FileValidator m_fileValidator;

private:
  /// Returns an error if at least one of the files is not valid, else "".
  virtual std::string checkValidity(const std::vector<std::vector<std::string> > &values) const;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_MULTIFILEVALIDATOR_H_*/
