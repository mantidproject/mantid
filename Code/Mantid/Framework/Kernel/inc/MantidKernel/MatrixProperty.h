#ifndef MANTID_KERNEL_MATRIXPROPERTY_H_
#define MANTID_KERNEL_MATRIXPROPERTY_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/Matrix.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/NullValidator.h"

namespace Mantid
{
  namespace Kernel
  {
    /*
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
    template<class TYPE = double>
    class DLLExport MatrixProperty : public PropertyWithValue<Matrix<TYPE> >
    {
      /// Typedef the held type
      typedef Kernel::Matrix<TYPE> HeldType;

    public:
      /// Constructor
      MatrixProperty(const std::string & propName,
                     IValidator_sptr validator = IValidator_sptr(new NullValidator),
                     unsigned int direction = Direction::Input);
      /// Copy constructor
      MatrixProperty(const MatrixProperty & rhs);
      // Unhide base class members (at minimum, avoids Intel compiler warning)
      using PropertyWithValue<HeldType>::operator=;
      /// 'Virtual copy constructor'
      inline MatrixProperty* clone() const { return new MatrixProperty(*this); }
      /// Destructor
      ~MatrixProperty();

      ///Add the value of another property. Doesn't make sense here.
      virtual MatrixProperty& operator+=( Kernel::Property const *)
      {
        throw Exception::NotImplementedError("+= operator is not implemented for MatrixProperty.");
        return *this;
      }

    private:
      /// Default constructor
      MatrixProperty();

    };

  }
}

#endif //MANTID_KERNEL_MATRIXPROPERTY_H_
