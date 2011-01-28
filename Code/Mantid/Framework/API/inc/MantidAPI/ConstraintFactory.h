#ifndef MANTID_API_CONSTRAINTFACTORY_H_
#define MANTID_API_CONSTRAINTFACTORY_H_

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
  class IConstraint;
  class IFitFunction;
  class Expression;

/** @class Mantid::API::ConstraintFactoryImpl

    The ConstraintFactory class is in charge of the creation of concrete
    instances of Constraints. It inherits most of its implementation from
    the Dynamic Factory base class.
    It is implemented as a singleton class.
    
    @author Roman Tolchenov, Tessella Support Services plc
    @date 4/02/2010
    
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

  class EXPORT_OPT_MANTID_API ConstraintFactoryImpl : public Kernel::DynamicFactory<IConstraint>
  {
  public:
    /**Creates an instance of a Constraint
     * @param fun :: The function
     * @param input :: The creation expression
     * @return A pointer to the created Constraint
     */
    IConstraint* createInitialized(IFitFunction* fun, const std::string& input) const;
    /**Creates an instance of a Constraint
     * @param fun :: The function
     * @param expr :: The creation expression
     * @return A pointer to the created Constraint
     */
    IConstraint* createInitialized(IFitFunction* fun, const Expression& expr) const;

  private:
    friend struct Mantid::Kernel::CreateUsingNew<ConstraintFactoryImpl>;

    /// Private Constructor for singleton class
    ConstraintFactoryImpl();
    /// Private copy constructor - NO COPY ALLOWED
    ConstraintFactoryImpl(const ConstraintFactoryImpl&);
    /// Private assignment operator - NO ASSIGNMENT ALLOWED
    ConstraintFactoryImpl& operator = (const ConstraintFactoryImpl&);
    ///Private Destructor
    virtual ~ConstraintFactoryImpl();

    ///static reference to the logger class
    Kernel::Logger& g_log;

  };
  
	///Forward declaration of a specialisation of SingletonHolder for AlgorithmFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
	template class EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<ConstraintFactoryImpl>;
#endif /* _WIN32 */
	typedef EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<ConstraintFactoryImpl> ConstraintFactory;
	
} // namespace API
} // namespace Mantid

/**
 * Macro for declaring a new type of function to be used with the FunctionFactory
 */
#define DECLARE_CONSTRAINT(classname) \
        namespace { \
	Mantid::Kernel::RegistrationHelper register_constraint_##classname( \
  ((Mantid::API::ConstraintFactory::Instance().subscribe<classname>(#classname)) \
	, 0)); \
	}

#endif /*MANTID_API_CONSTRAINTFACTORY_H_*/
