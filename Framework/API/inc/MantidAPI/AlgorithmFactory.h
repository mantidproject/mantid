// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include <memory>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Mantid {
namespace API {

/// Structure uniquely describing an algorithm with its name, category and
/// version.
struct AlgorithmDescriptor {
  std::string name;     ///< Algorithm Name
  int version;          ///< version
  std::string category; ///< category
  std::string alias;    ///< alias
};

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
class IAlgorithm;
class Algorithm;

/** The AlgorithmFactory class is in charge of the creation of concrete
    instances of Algorithms. It inherits most of its implementation from
    the Dynamic Factory base class.
    It is implemented as a singleton class.

    @author Russell Taylor, Tessella Support Services plc
    @date 21/09/2007
*/
class MANTID_API_DLL AlgorithmFactoryImpl final : public Kernel::DynamicFactory<Algorithm> {
public:
  AlgorithmFactoryImpl(const AlgorithmFactoryImpl &) = delete;
  AlgorithmFactoryImpl &operator=(const AlgorithmFactoryImpl &) = delete;
  // Unhide the base class version (to satisfy the intel compiler)
  using Kernel::DynamicFactory<Algorithm>::create;
  /// Creates an instance of an algorithm
  std::shared_ptr<Algorithm> create(const std::string &, const int &) const;

  /// algorithm factory specific function to subscribe algorithms, calls the
  /// dynamic factory subscribe function internally
  template <class C> std::pair<std::string, int> subscribe() {
    std::unique_ptr<Kernel::AbstractInstantiator<Algorithm>> newI =
        std::make_unique<Kernel::Instantiator<C, Algorithm>>();
    return this->subscribe(std::move(newI));
  }

  /**
   * Subscribes an algorithm using a custom instantiator. This
   * object takes ownership of the instantiator
   * @param instantiator - A pointer to a custom instantiator
   * @param replaceExisting - Defines what happens if an algorithm of the same
   * name/version already exists, see SubscribeAction
   * @returns The classname that was registered
   */
  template <class T>
  std::pair<std::string, int> subscribe(std::unique_ptr<Kernel::AbstractInstantiator<T>> instantiator,
                                        const SubscribeAction replaceExisting = ErrorIfExists) {
    const auto tempAlg = instantiator->createInstance();
    const int version = extractAlgVersion(tempAlg);
    const std::string className = extractAlgName(tempAlg);
    const std::string alias = extractAlgAlias(tempAlg);
    typename VersionMap::const_iterator it = m_vmap.find(className);
    if (!className.empty()) {
      const std::string key = createName(className, version);
      if (it == m_vmap.end()) {
        m_vmap[className] = version;
      } else {
        if (version == it->second && replaceExisting == ErrorIfExists) {
          std::ostringstream os;
          os << "Cannot register algorithm " << className << " twice with the same version\n";
          throw std::runtime_error(os.str());
        }
        if (version > it->second) {
          m_vmap[className] = version;
        }
      }
      Kernel::DynamicFactory<Algorithm>::subscribe(key, std::move(instantiator), replaceExisting);
    } else {
      throw std::invalid_argument("Cannot register empty algorithm name");
    }
    auto nameAndVersion = std::make_pair(className, version);

    if (!alias.empty())
      m_amap[alias] = nameAndVersion;

    return nameAndVersion;
  }
  /// Unsubscribe the given algorithm
  void unsubscribe(const std::string &algorithmName, const int version);
  /// Does an algorithm of the given name and version exist
  bool exists(const std::string &algorithmName, const int version = -1);

  /// Get the algorithm names and version - mangled use decodeName to separate
  const std::vector<std::string> getKeys() const override;
  const std::vector<std::string> getKeys(bool includeHidden) const;

  /// Get an algorithms name and version from the alias map
  std::optional<std::pair<std::string, int>> getRealNameFromAlias(const std::string &alias) const noexcept;

  /// Returns the highest version of the algorithm currently registered
  int highestVersion(const std::string &algorithmName) const;

  /// Get the algorithm categories
  const std::unordered_set<std::string> getCategories(bool includeHidden = false) const;

  /// Get the algorithm categories
  const std::map<std::string, bool> getCategoriesWithState() const;

  /// Returns algorithm descriptors.
  std::vector<AlgorithmDescriptor> getDescriptors(bool includeHidden = false, bool includeAliases = false) const;

  /// unmangles the names used as keys into the name and version
  std::pair<std::string, int> decodeName(const std::string &mangledName) const;

private:
  friend struct Mantid::Kernel::CreateUsingNew<AlgorithmFactoryImpl>;

  /// Extract the name of an algorithm
  const std::string extractAlgName(const std::shared_ptr<IAlgorithm> &alg) const;
  /// Extract the version of an algorithm
  int extractAlgVersion(const std::shared_ptr<IAlgorithm> &alg) const;
  /// Extract the alias of an algorithm
  const std::string extractAlgAlias(const std::shared_ptr<IAlgorithm> &alg) const;

  /// Create an algorithm object with the specified name
  std::shared_ptr<Algorithm> createAlgorithm(const std::string &name, const int version) const;

  /// Private Constructor for singleton class
  AlgorithmFactoryImpl();
  /// Private Destructor
  ~AlgorithmFactoryImpl() override;
  /// creates an algorithm name convolved from an name and version
  std::string createName(const std::string &, const int &) const;
  /// fills a set with the hidden categories
  void fillHiddenCategories(std::unordered_set<std::string> *categorySet) const;

  /// A typedef for the map of algorithm versions
  using VersionMap = std::map<std::string, int>;
  /// The map holding the registered class names and their highest versions
  VersionMap m_vmap;
  /// A typedef for the map of algorithm aliases
  using AliasMap = std::unordered_map<std::string, std::pair<std::string, int>>;
  /// The map holding the alias names of registered algorithms
  AliasMap m_amap;
};

using AlgorithmFactory = Mantid::Kernel::SingletonHolder<AlgorithmFactoryImpl>;

/// Convenient typedef for an UpdateNotification
using AlgorithmFactoryUpdateNotification = Mantid::Kernel::DynamicFactory<Algorithm>::UpdateNotification;
using AlgorithmFactoryUpdateNotification_ptr =
    const Poco::AutoPtr<Mantid::Kernel::DynamicFactory<Algorithm>::UpdateNotification> &;

} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<Mantid::API::AlgorithmFactoryImpl>;
}
} // namespace Mantid
