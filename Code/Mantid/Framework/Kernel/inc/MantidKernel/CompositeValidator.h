#ifndef MANTID_KERNEL_COMPOSITEVALIDATOR_H_
#define MANTID_KERNEL_COMPOSITEVALIDATOR_H_
    
#include "MantidKernel/System.h"
#include "MantidKernel/IValidator.h"
#include <vector>


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

    // ------------------------------------------------------------------------------------
    Kernel::IValidator<TYPE>* clone()
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
