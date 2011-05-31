#ifndef MANTID_API_ALGORITHMPROPERTY_H_
#define MANTID_API_ALGORITHMPROPERTY_H_

#include "MantidAPI/DllExport.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/NullValidator.h"

namespace Mantid
{
  namespace API
  {
    //-------------------------------------------------------------------------
    // Forward declarations
    //-------------------------------------------------------------------------
    class IAlgorithm;

    /**
    Define an algorithm property that can be used to supply an algorithm object 
    to a subsequent algorithm. It is a specialized version of PropertyWithValue
    where the type a pointer to an object implementing the 
    API::IAlgorithm interface.

    @author Martyn Gigg, Tessella Plc
    @data 24/03/2011

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
    class EXPORT_OPT_MANTID_API AlgorithmProperty : public Kernel::PropertyWithValue<boost::shared_ptr<IAlgorithm> >
    {
    public:
      /// Typedef the held type
      typedef boost::shared_ptr<IAlgorithm> HeldType; 

      /// Constructor
      AlgorithmProperty(const std::string & propName,
                        Kernel::IValidator<HeldType> *validator = new Kernel::NullValidator<HeldType>(), 
                        unsigned int direction = Kernel::Direction::Input);
      /// Copy constructor
      AlgorithmProperty(const AlgorithmProperty & rhs);
      // Unhide base class members (at minimum, avoids Intel compiler warning)
      using Kernel::PropertyWithValue<HeldType>::operator=;
      /// Copy-Assignment operator
      AlgorithmProperty& operator=(const AlgorithmProperty & rhs);
      /// 'Virtual copy constructor'
      inline Kernel::Property* clone() { return new AlgorithmProperty(*this); }
      /// Destructor
      ~AlgorithmProperty();

      ///Add the value of another property. Doesn't make sense here.
      virtual AlgorithmProperty& operator+=( Kernel::Property const *)
      {
        throw Kernel::Exception::NotImplementedError("+= operator is not implemented for AlgorithmProperty.");
        return *this;
      }
      /// Return the algorithm as string
      virtual std::string value() const;
      /// Get the default
      virtual std::string getDefault() const;
      /// Sets the value of the algorithm
      virtual std::string setValue(const std::string& value);

    private:
      /// Default constructor
      AlgorithmProperty();

      /// The string used to create the underlying algorithm
      std::string m_algStr;
    };
 } // namespace API
} // namespace Mantid

#endif  /* MANTID_API_ALGORITHMPROPERTY_H_ */
