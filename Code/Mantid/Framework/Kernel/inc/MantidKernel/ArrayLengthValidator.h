#ifndef MANTID_KERNEL_ARRAYLENGTHVALIDATOR_H_
#define MANTID_KERNEL_ARRAYLENGTHVALIDATOR_H_

#include "MantidKernel/IValidator.h"
#include <vector>


namespace Mantid
{
namespace Kernel
{

  /** ArrayLenghtValidator : Validate lenghth of an array property
    
    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  template <typename TYPE>
  class MANTID_KERNEL_DLL ArrayLengthValidator: public IValidator<std::vector<TYPE> >
  {
  public:
    ArrayLengthValidator();
    ArrayLengthValidator(const size_t len);
    virtual ~ArrayLengthValidator();

    IValidator<std::vector<TYPE> >* clone();

    std::string isValid(const std::vector<TYPE> &value) const;

    /// Return if it has a length
    bool hasLength() const;
    /// Return the length
    const size_t& getLength()    const;
    /// Set lower bound value
    void setLength( const size_t& value );
    /// Clear the length
    void clearLength();

  private:
    std::string checkValidity( const std::vector<TYPE> &value ) const;
    /// private variable containing the size of the array
    size_t m_arraySize;
    /// private variable, true if size is set, false if not
    bool m_hasArraySize;
  };


} // namespace Kernel
} // namespace Mantid

#endif  /* MANTID_KERNEL_ARRAYLENGTHVALIDATOR_H_ */
