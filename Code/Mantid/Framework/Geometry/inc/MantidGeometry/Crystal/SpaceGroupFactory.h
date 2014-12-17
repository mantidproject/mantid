#ifndef MANTID_GEOMETRY_SPACEGROUPFACTORY_H_
#define MANTID_GEOMETRY_SPACEGROUPFACTORY_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidGeometry/Crystal/SpaceGroup.h"
#include "MantidKernel/RegistrationHelper.h"

#include <map>

namespace Mantid {
namespace Geometry {

bool MANTID_GEOMETRY_DLL
    isValidGeneratorString(const std::string &generatorString);

/** AbstractSpaceGroupGenerator
 *
 * AbstractSpaceGroupGenerator is used by SpaceGroupFactory to delay
 * (possibly costly) construction of space group prototype objects until
 * they are actually requested. Instead of constructing a prototype of
 * a space group on registration, SpaceGroupFactory constructs an
 * AbstractSpaceGroupGenerator and stores that. Once a space group is requested,
 * a prototype is constructed - but only the first time.
 *
 * The Group that contains the symmetry operations for a certain space group
 * can be generated in different ways, so generateGroup is pure virtual and
 * leaves the door open for new algorithms that may be more efficient or have
 * other favorable properties.
 */
class MANTID_GEOMETRY_DLL AbstractSpaceGroupGenerator {
public:
  AbstractSpaceGroupGenerator(size_t number, const std::string &hmSymbol,
                              const std::string &generatorInformation);
  virtual ~AbstractSpaceGroupGenerator() {}

  inline size_t getNumber() const { return m_number; }
  inline std::string getHMSymbol() const { return m_hmSymbol; }
  inline std::string getGeneratorString() const { return m_generatorString; }

  SpaceGroup_const_sptr getPrototype();

protected:
  virtual Group_const_sptr generateGroup() const = 0;

private:
  inline bool hasValidPrototype() const {
    return static_cast<bool>(m_prototype);
  }
  SpaceGroup_const_sptr generatePrototype();

  size_t m_number;
  std::string m_hmSymbol;
  std::string m_generatorString;

  SpaceGroup_const_sptr m_prototype;
};

typedef boost::shared_ptr<AbstractSpaceGroupGenerator>
    AbstractSpaceGroupGenerator_sptr;

/// Concrete space group generator that uses space group generators as given in
/// ITA.
class MANTID_GEOMETRY_DLL AlgorithmicSpaceGroupGenerator
    : public AbstractSpaceGroupGenerator {
public:
  AlgorithmicSpaceGroupGenerator(size_t number, const std::string &hmSymbol,
                                 const std::string &generatorInformation);
  virtual ~AlgorithmicSpaceGroupGenerator() {}

protected:
  Group_const_sptr generateGroup() const;
  std::string getCenteringSymbol() const;
};

/// Concrete space group generator that constructs space groups from a list of
/// symmetry operations with no further computations.
class MANTID_GEOMETRY_DLL TabulatedSpaceGroupGenerator
    : public AbstractSpaceGroupGenerator {
public:
  TabulatedSpaceGroupGenerator(size_t number, const std::string &hmSymbol,
                               const std::string &generatorInformation);
  virtual ~TabulatedSpaceGroupGenerator() {}

protected:
  Group_const_sptr generateGroup() const;
};

/** SpaceGroupFactory

  This factory is used to create space group objects. Each space group
  should be created only once, which is why the factory works with
  prototypes that are cloned when a space group is requested more than once.

  On the other hand, these prototypes should not be constructed when they are
  registered, because that might slow down startup of Mantid. As a solution,
  space groups are not stored in the factory directly, instead
  a generator is constructed on registration, which will do the prototype
  construction when it's actually required - and still only once.

  In principle, any generator can be used through the template method
  subscribeUsingGenerator, which constructs the object and stores it as
  a base class pointer. For convenience there are two methods which
  provide a generator- and a table-based approach
  (subscribeGeneratedSpaceGroup and subscribeTabulatedSpaceGroup).

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 08/10/2014

    Copyright © 2014 PSI-MSS

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
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_GEOMETRY_DLL SpaceGroupFactoryImpl {
public:
  virtual ~SpaceGroupFactoryImpl() {}

  SpaceGroup_const_sptr createSpaceGroup(const std::string &hmSymbol);

  bool isSubscribed(const std::string &hmSymbol) const;
  bool isSubscribed(size_t number) const;

  std::vector<std::string> subscribedSpaceGroupSymbols() const;
  std::vector<std::string> subscribedSpaceGroupSymbols(size_t number) const;
  std::vector<size_t> subscribedSpaceGroupNumbers() const;

  void unsubscribeSpaceGroup(const std::string &hmSymbol);

  void subscribeGeneratedSpaceGroup(size_t number, const std::string &hmSymbol,
                                    const std::string &generators);
  void subscribeTabulatedSpaceGroup(size_t number, const std::string &hmSymbol,
                                    const std::string &symmetryOperations);

  /// Templated method to subscribe other generators than the ones provided
  /// here.
  template <typename T>
  void subscribeUsingGenerator(size_t number, const std::string &hmSymbol,
                               const std::string &generatorString) {
    if (isSubscribed(hmSymbol)) {
      throw std::invalid_argument(
          "Space group with this symbol is already registered.");
    }

    AbstractSpaceGroupGenerator_sptr generator =
        boost::make_shared<T>(number, hmSymbol, generatorString);

    subscribe(generator);
  }

protected:
  SpaceGroup_const_sptr getPrototype(const std::string &hmSymbol);
  void subscribe(const AbstractSpaceGroupGenerator_sptr &generator);
  SpaceGroup_const_sptr
  constructFromPrototype(const SpaceGroup_const_sptr prototype) const;

  std::multimap<size_t, std::string> m_numberMap;
  std::map<std::string, AbstractSpaceGroupGenerator_sptr> m_generatorMap;

  SpaceGroupFactoryImpl();

private:
  friend struct Mantid::Kernel::CreateUsingNew<SpaceGroupFactoryImpl>;
};

// This is taken from FuncMinimizerFactory
#ifdef _WIN32
template class MANTID_GEOMETRY_DLL
    Mantid::Kernel::SingletonHolder<SpaceGroupFactoryImpl>;
#endif

typedef Mantid::Kernel::SingletonHolder<SpaceGroupFactoryImpl>
    SpaceGroupFactory;

} // namespace Geometry
} // namespace Mantid

/* Macros for compile time space group registration
 *
 * The macros are a bit different than in other factories,
 * because there is no identifier that can be used to generate
 * a unique name for each RegistrationHelper instance.
 *
 * Instead, the __COUNTER__ macro is used, which is available
 * in many compilers and is incremented every time it's called.
 *
 * Solution was found here: http://stackoverflow.com/a/1295338
 */
#define SPGF_CONCAT_IMPL(x, y) x##y
#define SPGF_CONCAT(x, y) SPGF_CONCAT_IMPL(x, y)

#define DECLARE_GENERATED_SPACE_GROUP(number, hmSymbol, generators)            \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper SPGF_CONCAT(register_spacegroup_,         \
                                                 __COUNTER__)(                 \
      ((Mantid::Geometry::SpaceGroupFactory::Instance()                        \
            .subscribeGeneratedSpaceGroup(number, hmSymbol, generators)),      \
       0));                                                                    \
  }

#define DECLARE_TABULATED_SPACE_GROUP(number, hmSymbol, symmetryOperations)    \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper SPGF_CONCAT(register_spacegroup_,         \
                                                 __COUNTER__)(                 \
      ((Mantid::Geometry::SpaceGroupFactory::Instance()                        \
            .subscribeTabulatedSpaceGroup(number, hmSymbol,                    \
                                          symmetryOperations)),                \
       0));                                                                    \
  }

#endif /* MANTID_GEOMETRY_SPACEGROUPFACTORY_H_ */
