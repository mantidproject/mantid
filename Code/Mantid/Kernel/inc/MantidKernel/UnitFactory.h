#ifndef MANTID_KERNEL_UNITFACTORYIMPL_H_
#define MANTID_KERNEL_UNITFACTORYIMPL_H_

/* Used to register unit classes into the factory. creates a global object in an
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's
 * subscribe method.
 *
 * The second operation that this macro performs is to provide the definition
 * of the unitID method for the concrete unit.
 */
#define DECLARE_UNIT(classname) \
  namespace { \
    Mantid::Kernel::RegistrationHelper register_alg_##classname( \
       ((Mantid::Kernel::UnitFactory::Instance().subscribe<classname>(#classname)) \
       , 0)); \
  } \
  const std::string Mantid::Kernel::Units::classname::unitID() const {return #classname;}


//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllExport.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid
{
namespace Kernel
{

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class Unit;
class Logger;

/** Creates instances of concrete units.
    The factory is a singleton that hands out shared pointers to the base Unit class.
    It overrides the base class DynamicFactory::create method so that only a single
    instance of a given unit is ever created, and a pointer to that same instance
    is passed out each time the unit is requested.

    @author Russell Taylor, Tessella Support Services plc
    @date 13/03/2008

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class EXPORT_OPT_MANTID_KERNEL UnitFactoryImpl : public DynamicFactory<Unit>
{
public:
  virtual boost::shared_ptr<Unit> create(const std::string& className) const;

private:
  friend struct CreateUsingNew<UnitFactoryImpl>;

  /// Private Constructor for singleton class
  UnitFactoryImpl();
  /// Private copy constructor - NO COPY ALLOWED
  UnitFactoryImpl(const UnitFactoryImpl&);
  /// Private assignment operator - NO ASSIGNMENT ALLOWED
  UnitFactoryImpl& operator = (const UnitFactoryImpl&);
  ///Private Destructor
  virtual ~UnitFactoryImpl();

  /// Stores pointers to already created unit instances, with their name as the key
  mutable std::map< std::string, boost::shared_ptr<Unit> > m_createdUnits;

  /// Reference to the logger class
  Kernel::Logger& m_log;
};

///Forward declaration of a specialisation of SingletonHolder for AlgorithmFactoryImpl (needed for dllexport/dllimport) .
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
  template class EXPORT_OPT_MANTID_KERNEL Mantid::Kernel::SingletonHolder<UnitFactoryImpl>;
#endif /* _WIN32 */
/// The specialisation of the SingletonHolder class that holds the UnitFactory
typedef SingletonHolder<UnitFactoryImpl> UnitFactory;

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_UNITFACTORYIMPL_H_*/
