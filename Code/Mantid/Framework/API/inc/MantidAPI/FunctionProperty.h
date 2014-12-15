#ifndef MANTID_API_FUNCTIONPROPERTY_H_
#define MANTID_API_FUNCTIONPROPERTY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/Exception.h"

#include <string>

namespace Mantid
{
  namespace API
  {
    /** A property class for functions. Holds a shared pointer to a function. The string representation
    is the creation string accepted by the FunctionFactory.

    @author Roman Tolchenov, Tessella plc
    @date 08/02/2012

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class MANTID_API_DLL FunctionProperty : public Kernel::PropertyWithValue< boost::shared_ptr<IFunction> >
    {
    public:
      /// Constructor.
      explicit FunctionProperty( const std::string &name );

      /// Copy constructor
      FunctionProperty( const FunctionProperty& right );

      /// Copy assignment operator. Only copies the value (i.e. the pointer to the function)
      FunctionProperty& operator=( const FunctionProperty& right );

      /// Bring in the PropertyWithValue assignment operator explicitly (avoids VSC++ warning)
      virtual boost::shared_ptr<IFunction>& operator=( const boost::shared_ptr<IFunction>& value );

      ///Add the value of another property
      virtual FunctionProperty& operator+=( Kernel::Property const * );

      /// 'Virtual copy constructor'
      FunctionProperty* clone() const { return new FunctionProperty(*this); }

      /// Virtual destructor
      virtual ~FunctionProperty();

      /// Get the function definition string
      virtual std::string value() const;

      /// Get the value the property was initialised with -its default value
      virtual std::string getDefault() const;

      /// Set the value of the property. 
      virtual std::string setValue( const std::string& value );

      /// Checks whether the entered function is valid.
      std::string isValid() const;

      /// Is default
      bool isDefault() const;

      /// Create a history record
      virtual const Kernel::PropertyHistory createHistory() const;

    private:

      /// The function definition string (as used by the FunctionFactory)
      std::string m_definition;
    };


  } // namespace API
} // namespace Mantid

#endif /*MANTID_API_FUNCTIONPROPERTY_H_*/
