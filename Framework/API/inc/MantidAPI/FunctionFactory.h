#ifndef MANTID_API_FUNCTIONFACTORY_H_
#define MANTID_API_FUNCTIONFACTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

#include <mutex>

namespace Mantid {

namespace API {

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

    Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
*/

class MANTID_API_DLL FunctionFactoryImpl final
    : public Kernel::DynamicFactory<IFunction> {
public:
  FunctionFactoryImpl(const FunctionFactoryImpl &) = delete;
  FunctionFactoryImpl &operator=(const FunctionFactoryImpl &) = delete;
  /**Creates an instance of a function
   * @param type :: The function's type
   * @return A pointer to the created function
   */
  boost::shared_ptr<IFunction> createFunction(const std::string &type) const;

  /// Creates an instance of a function
  boost::shared_ptr<IFunction>
  createInitialized(const std::string &input) const;

  /// Query available functions based on the template type
  template <typename FunctionType>
  const std::vector<std::string> &getFunctionNames() const;
  // Unhide the base class version (to satisfy the intel compiler)
  using Kernel::DynamicFactory<IFunction>::subscribe;
  void subscribe(const std::string &className,
                 AbstractFactory *pAbstractFactory,
                 Kernel::DynamicFactory<IFunction>::SubscribeAction replace =
                     ErrorIfExists);

  void unsubscribe(const std::string &className);

private:
  friend struct Mantid::Kernel::CreateUsingNew<FunctionFactoryImpl>;

  /// Private Constructor for singleton class
  FunctionFactoryImpl();
  /// Private Destructor
  ~FunctionFactoryImpl() override = default;
  /// These methods shouldn't be used to create functions
  using Kernel::DynamicFactory<IFunction>::create;
  using Kernel::DynamicFactory<IFunction>::createUnwrapped;

  /// Create a simple function
  boost::shared_ptr<IFunction>
  createSimple(const Expression &expr,
               std::map<std::string, std::string> &parentAttributes) const;
  /// Create a composite function
  boost::shared_ptr<CompositeFunction>
  createComposite(const Expression &expr,
                  std::map<std::string, std::string> &parentAttributes) const;

  /// Throw an exception
  void inputError(const std::string &str = "") const;
  /// Add constraints to the created function
  void addConstraints(boost::shared_ptr<IFunction> fun,
                      const Expression &expr) const;
  /// Add a single constraint to the created function
  void addConstraint(boost::shared_ptr<IFunction> fun,
                     const Expression &expr) const;
  /// Add ties to the created function
  void addTies(boost::shared_ptr<IFunction> fun, const Expression &expr) const;
  /// Add a tie to the created function
  void addTie(boost::shared_ptr<IFunction> fun, const Expression &expr) const;

  mutable std::map<std::string, std::vector<std::string>> m_cachedFunctionNames;
  mutable std::mutex m_mutex;
};

/**
 * Query available functions based on the template type
 * @tparam FunctionType :: The type of the functions to list
 * @returns A vector of the names of the functions matching the template type
 */
template <typename FunctionType>
const std::vector<std::string> &FunctionFactoryImpl::getFunctionNames() const {
  std::lock_guard<std::mutex> _lock(m_mutex);

  const std::string soughtType(typeid(FunctionType).name());
  if (m_cachedFunctionNames.find(soughtType) != m_cachedFunctionNames.end()) {
    return m_cachedFunctionNames[soughtType];
  }

  // Create the entry in the cache and work with it directly
  std::vector<std::string> &typeNames = m_cachedFunctionNames[soughtType];
  const std::vector<std::string> names = this->getKeys();
  std::copy_if(names.cbegin(), names.cend(), std::back_inserter(typeNames),
               [this](const std::string &name) {
                 boost::shared_ptr<IFunction> func = this->createFunction(name);
                 return boost::dynamic_pointer_cast<FunctionType>(func);
               });
  return typeNames;
}

using FunctionFactory = Mantid::Kernel::SingletonHolder<FunctionFactoryImpl>;

/// Convenient typedef for an UpdateNotification
using FunctionFactoryUpdateNotification =
    FunctionFactoryImpl::UpdateNotification;
/// Convenient typedef for an UpdateNotification AutoPtr
using FunctionFactoryUpdateNotification_ptr =
    const Poco::AutoPtr<FunctionFactoryUpdateNotification> &;
} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<Mantid::API::FunctionFactoryImpl>;
}
}

/**
 * Macro for declaring a new type of function to be used with the
 * FunctionFactory
 */
#define DECLARE_FUNCTION(classname)                                            \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper                                           \
      register_function_##classname(((Mantid::API::FunctionFactory::Instance() \
                                          .subscribe<classname>(#classname)),  \
                                     0));                                      \
  }

#endif /*MANTID_API_FUNCTIONFACTORY_H_*/
