#ifndef MANTID_KERNEL_MASKEDPROPERTY_H_
#define MANTID_KERNEL_MASKEDPROPERTY_H_

#include "MantidKernel/PropertyWithValue.h"


 /** A property class for masking the properties. It inherits from PropertyWithValue.
     This class masks the properties and useful when the property value is not to be
	 displayed  in the user interface widgets like algorithm history display,
	 log window etc and log file .The masked property will be displayed.
     This class is specialised for string and masks the property value with charcater "*"
    
    @class Mantid::Kernel::MaskedProperty

    Copyright &copy;  2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

namespace Mantid
{
	namespace Kernel
	{
		template <typename TYPE = std::string>
		class MaskedProperty: public Kernel::PropertyWithValue<TYPE >
		{
		public:
		  /** Constructor  for Maskedproperty class
			* @param name - name of the property
			* @param defaultvalue - defaultvalue of the property
			* @param validator - property validator
			* @param direction - Whether this is a Direction::Input, Direction::Output or Direction::InOut (Input & Output) property
			*/
			  MaskedProperty(const std::string& name,TYPE defaultvalue,IValidator<TYPE> *validator = new NullValidator<TYPE>,const unsigned int direction = Direction::Input):
			  Kernel::PropertyWithValue<TYPE>(name,defaultvalue , validator, direction ),m_maskedValue("")
			  { 
			  }

			/** Constructor  for Maskedproperty class
			  * @param name - name of the property
			  * @param defaultvalue - defaultvalue of the property
			  * @param direction - Whether this is a Direction::Input, Direction::Output or Direction::InOut (Input & Output) property
			  */
			  MaskedProperty( const std::string& name, const TYPE& defaultvalue, const unsigned int direction):
			  Kernel::PropertyWithValue <TYPE>(name,defaultvalue,direction ),m_maskedValue("")
			  {
				  
			  }
			 
			/** This method creates History 
			  */
			  virtual const Kernel::PropertyHistory createHistory() const
			  {
							  
				  //doMasking();
				
				  return Kernel::PropertyHistory(this->name(),this->getMaskedValue(),this->type(),this->isDefault(),Kernel::PropertyWithValue<TYPE >::direction());
			  }

			  /** This method returns the masked property value
			  */
			  TYPE getMaskedValue() const
			  {	
				  doMasking();
				  return m_maskedValue;
			  }
        /**
        * Do not remember the inputs from a masked property
        * @return do not remember masked property values
        */
        virtual bool remember() const { return false; };
		private:

			/** This method creates masked value for a property
			  * Useful for masking for properties like password in mantid
			  */
			  void doMasking()const 
			  {
				  TYPE value(this->value());
				  m_maskedValue=std::string(value.size(),'*');
			  }

			
		private:
			mutable TYPE m_maskedValue; ///< the masked value

		};
	}
}
#endif
