#ifndef MANTID_KERNEL_VALIDATOR_SIGNAL_CHANGE_H_
#define MANTID_KERNEL_VALIDATOR_SIGNAL_CHANGE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/Logger.h"
#include "boost/signal.hpp"
//#include <string>

namespace Mantid
{
namespace Kernel
{

/** ValidatorSignalChange is a validator, which connects to a property and signals to subscribers that this property has changed

   DO NOT USE -- it is currently the concept, which has memory leak. 
    @date 03/11/2011

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
template <typename TYPE>
class DLLExport ValidatorSignalChange : public IValidator< TYPE >, public boost::signal<std::string (Property const *const)>
{
public:
  /// Constructor; takes property to check
  ValidatorSignalChange(Property const *const pProp):
      IValidator<TYPE>() ,
      boost::signal<std::string (Property const* const)>(),
      pPropObserved(pProp)
  {}

  ///virtual Destructor
  virtual ~ValidatorSignalChange() {}

  //------------------------------------------------------------------------------------------------------------
  /** Calls the validator, but validates nothing. Sends signal to subscribers instead
   *  Returns the results of the function, which is excecuted, when the signal has been called;
   */
  std::string isValid(const TYPE &) const
  {     
      return  this->operator()(pPropObserved);
  }

  //------------------------------------------------------------------------------------------------------------
  /** Does not verify allowed values
   *  @return The set of allowed values of this validato is an empty set
   */
  std::set<std::string> allowedValues() const { return std::set<std::string>(); }
   //
   /** Make a copy of the present type of validator  */
  IValidator<TYPE>* clone(){return new ValidatorSignalChange<TYPE>(pPropObserved);}
  //
protected:
    /// always valid
    std::string checkValidity(const TYPE &) const{return std::string("");}
private:
    // the pointer to property, this validator checks.
    Property const *const pPropObserved;
    //
};
} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_IVALIDATOR_H_*/