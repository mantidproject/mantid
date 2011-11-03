#ifndef MANTID_KERNEL_VALIDATOR_SIGNAL_CHANGE_H_
#define MANTID_KERNEL_VALIDATOR_SIGNAL_CHANGE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/Logger.h"
#include "boost/Signal.hpp"
//#include <string>

namespace Mantid
{
namespace Kernel
{

/** ValidatorSignalChange is a validator, which connects to a property and signals to subscribers that this property has changed

    @author Alex Buts
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
class DLLExport ValidatorSignalChange : public IValidator<TYPE>, public boost::signal<void (const Property *)>
{
public:
  /// Constructor
  ValidatorSignalChange(const Property *pProp):
      pPropObserved(pProp)
  {}

  ///virtual Destructor
  virtual ~ValidatorSignalChange() {}

  //------------------------------------------------------------------------------------------------------------
  /** Calls the validator
   *  Always valid
   */
  std::string isValid(const TYPE &value) const{
      this->operator()(pPropObserved);
      return "";
  }

  //------------------------------------------------------------------------------------------------------------
  /** Does not verify allowed values
   *  @return The set of allowed values that this validator may have or an empty set
   */
  std::set<std::string> allowedValues() const { return std::set<std::string>(); }
   //
   /** Make a copy of the present type of validator  */
  IValidator<TYPE>* clone(){return static_cast<IValidator<TYPE>* >(NULL);}
  //
protected:
    /// always valid
    std::string checkValidity(const TYPE &) const{return std::string("");}
private:
    const Property *pPropObserved;
 // boost::signal<void (Property  *)> *pSignal;

};
} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_IVALIDATOR_H_*/
