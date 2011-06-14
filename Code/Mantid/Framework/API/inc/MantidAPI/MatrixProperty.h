#ifndef MANTID_API_MATRIXPROPERTY_H_
#define MANTID_API_MATRIXPROPERTY_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/NullValidator.h"

namespace Mantid
{
  namespace API
  {
    /*
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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    template<class TYPE = double>
    class DLLExport MatrixProperty : public Kernel::PropertyWithValue<Geometry::Matrix<TYPE> >
    {
      /// Typedef the held type
      typedef Geometry::Matrix<TYPE> HeldType;

    public:
      /// Constructor
      MatrixProperty(const std::string & propName,
                        Kernel::IValidator<HeldType> *validator = new Kernel::NullValidator<HeldType>(), 
                        unsigned int direction = Kernel::Direction::Input);
      /// Copy constructor
      MatrixProperty(const MatrixProperty & rhs);
      // Unhide base class members (at minimum, avoids Intel compiler warning)
      using Kernel::PropertyWithValue<HeldType>::operator=;
      /// 'Virtual copy constructor'
      inline Kernel::Property* clone() { return new MatrixProperty(*this); }
      /// Destructor
      ~MatrixProperty();

      ///Add the value of another property. Doesn't make sense here.
      virtual MatrixProperty& operator+=( Kernel::Property const *)
      {
        throw Kernel::Exception::NotImplementedError("+= operator is not implemented for MatrixProperty.");
        return *this;
      }

    private:
      /// Default constructor
      MatrixProperty();

    };

  }
}

#endif //MANTID_API_MATRIXPROPERTY_H_
