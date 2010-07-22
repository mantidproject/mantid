#ifndef MANTID_API_FUNCTIONFACTORY_H_
#define MANTID_API_FUNCTIONFACTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidAPI/DllExport.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid
{
	
//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace Kernel
{
  class Logger;
}

namespace API
{

//----------------------------------------------------------------------
// More forward declarations
//----------------------------------------------------------------------
  class IFunction;
  class CompositeFunction;
  class Expression;

/** @class FunctionFactoryImpl

    The FunctionFactory class is in charge of the creation of concrete
    instances of fitting functions. It inherits most of its implementation from
    the Dynamic Factory base class.
    It is implemented as a singleton class.
    
    @author Roman Tolchenov, Tessella Support Services plc
    @date 27/10/2009
    
    Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
*/

  class EXPORT_OPT_MANTID_API FunctionFactoryImpl : public Kernel::DynamicFactory<IFunction>
  {
  public:
    /**Creates an instance of a function
     * @param type The function's type
     * @return A pointer to the created function
     */
    IFunction* createFunction(const std::string& type) const;

    ///Creates an instance of a function
    IFunction* createInitialized(const std::string& input) const;

  private:
    friend struct Mantid::Kernel::CreateUsingNew<FunctionFactoryImpl>;

    /// Private Constructor for singleton class
    FunctionFactoryImpl();
    /// Private copy constructor - NO COPY ALLOWED
    FunctionFactoryImpl(const FunctionFactoryImpl&);
    /// Private assignment operator - NO ASSIGNMENT ALLOWED
    FunctionFactoryImpl& operator = (const FunctionFactoryImpl&);
    ///Private Destructor
    virtual ~FunctionFactoryImpl();

    /// Create a simple function
    IFunction* createSimple(const Expression& expr)const;
    /// Create a composite function
    CompositeFunction* createComposite(const Expression& expr)const;
    /// Throw an exception
    void inputError(const std::string& str="")const;
    /// Add constraints to the created function
    void addConstraints(IFunction* fun,const Expression& expr)const;
    /// Add a single constraint to the created function
    void addConstraint(IFunction* fun,const Expression& expr)const;
    /// Add ties to the created function
    void addTies(IFunction* fun,const Expression& expr)const;
    /// Add a tie to the created function
    void addTie(IFunction* fun,const Expression& expr)const;

    ///static reference to the logger class
    Kernel::Logger& g_log;

  };
  
	///Forward declaration of a specialisation of SingletonHolder for AlgorithmFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
	template class EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<FunctionFactoryImpl>;
#endif /* _WIN32 */
	typedef EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<FunctionFactoryImpl> FunctionFactory;
	
} // namespace API
} // namespace Mantid

#endif /*MANTID_API_FUNCTIONFACTORY_H_*/
