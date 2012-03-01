#ifndef MANTID_KERNEL_COMPOSITEVALIDATOR_H_
#define MANTID_KERNEL_COMPOSITEVALIDATOR_H_
    
#include "MantidKernel/System.h"
#include "MantidKernel/IValidator.h"
#include <vector>
#include <set>


namespace Mantid
{
namespace Kernel
{


  //===============================================================================================
  /** A composite validator that can combine any 2+ arbitrary validators together.

      @author Russell Taylor, Janik Zikovsky
      @date Aug 25, 2011

      Copyright &copy; 2008-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport CompositeValidator : public Kernel::IValidator<TYPE>
  {
  public:
    CompositeValidator() {}

    virtual ~CompositeValidator()
    {
      for (unsigned int i=0; i < m_children.size(); ++i)
      {
        delete m_children[i];
      }
      m_children.clear();
    }

    // IValidator methods
    ///Gets the type of the validator
    std::string getType() const { return "composite"; }
    /** returns allowed values as intersection of non-empty sets of allowed values for child validators; 
     *
     * not very consistent but reasonable as non-list validators often return empty sets of allowed values;
     * primary purpose -- return only one set of values from single list validator placed within a composite validator;   */
    std::set<std::string> allowedValues() const 
    {
      std::set<std::string>      elem_unique;
      std::multiset<std::string> elem_all;
      // how many validators return non-empty list of allowed values
      int n_combinations(0);
      for (unsigned int i=0; i < m_children.size(); ++i)
      {
         std::set<std::string> subs = m_children[i]->allowedValues();
         if ( subs.empty())continue;
           elem_unique.insert(subs.begin(),subs.end());
           elem_all.insert(subs.begin(),subs.end());
           n_combinations++;
      }
 

      // empty or single set of allowed values
      if(n_combinations<2)return elem_unique;
      // there is more then one combination and we have to identify its union;   
      for(std::set<std::string>::const_iterator its=elem_unique.begin();its!=elem_unique.end();its++){
          std::multiset<std::string>::iterator im = elem_all.find(*its);
          elem_all.erase(im);
      }
      std::set<std::string> rez;
      for(std::multiset<std::string>::const_iterator im=elem_all.begin();im!=elem_all.end();im++){
          rez.insert(*im);
      }
      return rez;
       
    }
    // ------------------------------------------------------------------------------------
    Kernel::IValidator<TYPE>* clone() const
    {
      CompositeValidator<TYPE>* copy = new CompositeValidator<TYPE>();
      for (unsigned int i=0; i < m_children.size(); ++i)
      {
        copy->add( m_children[i]->clone() );
      }
      return copy;
    }

    // ------------------------------------------------------------------------------------
    /** Adds a validator to the group of validators to check
     *  @param child :: A pointer to the validator to add
     */
    void add(Kernel::IValidator<TYPE>* child)
    {
      m_children.push_back(child);
    }

     virtual void modify_validator(IValidator<TYPE> *pNewValidator)
     {
         UNUSED_ARG(pNewValidator);
     }
  private:
    /// Private Copy constructor: NO DIRECT COPY ALLOWED
    CompositeValidator(const CompositeValidator&);

    /** Checks the value of all child validators. Fails if any child fails.
     *  @param value :: The workspace to test
     *  @return A user level description of the first problem it finds otherwise ""
     */
    std::string checkValidity( const TYPE & value ) const
    {
      //Go though all the validators
      for (unsigned int i=0; i < m_children.size(); ++i)
      {
        std::string error = m_children[i]->isValid(value);
        //exit on the first error, to avoid passing doing more tests on invalid objects that could fail
        if (error != "") return error;
      }
      //there were no errors
      return "";
    }

    /// A container for the child validators
    std::vector<Kernel::IValidator<TYPE>*> m_children;
  };


} // namespace Kernel
} // namespace Mantid

#endif  /* MANTID_KERNEL_COMPOSITEVALIDATOR_H_ */
