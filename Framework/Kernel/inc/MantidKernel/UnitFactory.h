// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
#define DECLARE_UNIT(classname)                                                \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper                                           \
      register_unit_##classname(((Mantid::Kernel::UnitFactory::Instance()      \
                                      .subscribe<classname>(#classname)),      \
                                 0));                                          \
  }                                                                            \
  const std::string Mantid::Kernel::Units::classname::unitID() const {         \
    return #classname;                                                         \
  }

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Unit.h"

namespace Mantid {
namespace Kernel {

/** Creates instances of concrete units.
    The factory is a singleton that hands out shared pointers to the base Unit
   class.
    It overrides the base class DynamicFactory::create method so that only a
   single
    instance of a given unit is ever created, and a pointer to that same
   instance
    is passed out each time the unit is requested.

    @author Russell Taylor, Tessella Support Services plc
    @date 13/03/2008
*/
class MANTID_KERNEL_DLL UnitFactoryImpl final : public DynamicFactory<Unit> {
public:
  UnitFactoryImpl(const UnitFactoryImpl &) = delete;
  UnitFactoryImpl &operator=(const UnitFactoryImpl &) = delete;

  /// Returns the names of the convertible units in the factory
  /// @return A string vector of keys
  const std::vector<std::string> getConvertibleUnits() const {
    std::vector<std::string> convertibleUnits;

    for (const auto &unitKey : getKeys()) {
      auto unit_sptr = create(unitKey);
      if (unit_sptr->isConvertible()) {
        convertibleUnits.emplace_back(unitKey);
      }
    }

    return convertibleUnits;
  }

private:
  friend struct CreateUsingNew<UnitFactoryImpl>;

  /// Private Constructor for singleton class
  UnitFactoryImpl() = default;

  /// Private Destructor
  ~UnitFactoryImpl() override = default;
};

EXTERN_MANTID_KERNEL template class MANTID_KERNEL_DLL
    Mantid::Kernel::SingletonHolder<UnitFactoryImpl>;

using UnitFactory = SingletonHolder<UnitFactoryImpl>;

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_UNITFACTORYIMPL_H_*/
