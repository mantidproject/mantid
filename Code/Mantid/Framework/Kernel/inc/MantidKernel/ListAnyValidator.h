#ifndef MANTID_KERNEL_LIST_ANY_VALIDATOR_H_
#define MANTID_KERNEL_LIST_ANY_VALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "IValidator.h"
#include <vector>
#include <set>
#include <boost/lexical_cast.hpp>
#include <boost/type_traits.hpp>

namespace Mantid
{
namespace Kernel
{
/** ListAnyValidator is a validator that requires the value of a property to be one of a defined list
    of possibilities. This is generic list option. 

     @date 09/12/2011
 
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

// templated structure which implements compiler-time check if two template parameters are equal
template<typename T,typename U>
struct type_is_equal{
    static const bool value = false;
};
template<typename T>
struct type_is_equal<T,T>{
    static const bool value = true;
};


template<typename TYPE=std::string>
class  ListAnyValidator : public IValidator<TYPE>
{
private:
  // templated function which substitutes different code as function of the initiated type
  // this codes is substituted if the condifion is true
  /** Templated function which substitutes different code as function of the initiated type
   * @tparam U :: type of the input parameter
   * @tparam Condition :: set to True if type of U == type of TYPE,
   *         via the type_is_equal<T,U> structure.
   */
  template<typename U, bool Condition >
  class IF
  {
  public:
    /** Insert the value U directly into the validator, because U == TYPE
     *
     * @param pHost :: ListValidator
     * @param value :: value to insert, type U
     */
    static inline void ENABLE(ListAnyValidator *pHost,const U &value)
    {
      pHost->m_allowedValues.insert(value);
    }
  };

  // and this one -- if false
  template<typename U>
  class IF<U, false >
  {
  public:
    /** Cast the value U to TYPE and then insert into the validator
     *
     * @param pHost :: ListValidator
     * @param value :: value to insert, type U
     */
    static inline void ENABLE(ListAnyValidator *pHost,const U &value)
    {
      TYPE rVal = boost::lexical_cast<TYPE>(value);
      pHost->m_allowedValues.insert(rVal);
    }
  };

public:

    /// Default constructor. Sets up an empty list of valid values.
    ListAnyValidator(): IValidator<TYPE>(), m_allowedValues(){};

    /** Constructor
     *  @param values :: A set of values consisting of the valid values     */
    explicit ListAnyValidator(const std::set<TYPE>& values):
    IValidator<TYPE>(),  m_allowedValues(values){}

    /** Constructor
     *  @param values :: A vector of the valid values     */
    explicit ListAnyValidator(const std::vector<TYPE>& values):
    IValidator<TYPE>(), m_allowedValues(values.begin(),values.end()){}
    
    /// Destructor
    virtual ~ListAnyValidator(){};  

    /// Returns the set of valid values (as strings)
    virtual std::set<std::string> allowedValues() const
    {
      std::set<std::string> rez;
      typename std::set<TYPE >::const_iterator it=m_allowedValues.begin();
      for(;it!=m_allowedValues.end();it++){
          rez.insert(rez.end(),boost::lexical_cast<std::string>(*it));
      }
      return rez;         
    }

    /** Adds the argument to the set of valid values regardless of its type.
     * if the template type corresponds to  the class type, insertion goes directly,
     * if the template type is different -- lexical cast of the inserted values occurs    */
    template <typename U>
    void addAllowedValue(const U &value)
    {
      IF<U, type_is_equal<TYPE,U>::value >::ENABLE(this,value);
    }

    /// Clone this validator
  IValidator_sptr clone() const;

protected:
    /** Checks if the value passed is in the list
     *  @param value :: The value to test
     *  @return "" if the value is on the list, or "The value is not in the list of allowed values"   */
    virtual std::string checkValidity(const TYPE &value) const
    {
      if ( m_allowedValues.count(value) )
      {
        return "";
      }
      else
      {
        return "The value \"" + boost::lexical_cast<std::string>(value) + "\" is not in the list of allowed values";
      }
    }

  /// The set of valid values
  std::set<TYPE> m_allowedValues;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_LISTVALIDATOR_H_*/
