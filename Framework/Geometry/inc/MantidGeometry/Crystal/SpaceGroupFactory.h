// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/SpaceGroup.h"
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/RegistrationHelper.h"
#include "MantidKernel/SingletonHolder.h"

#include <map>

namespace Mantid {
namespace Geometry {

bool MANTID_GEOMETRY_DLL isValidGeneratorString(const std::string &generatorString);

std::vector<std::string> MANTID_GEOMETRY_DLL operator*(const SymmetryOperation &symOp,
                                                       const std::vector<std::string> &strings);

/**
 * @class AbstractSpaceGroupGenerator
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
  AbstractSpaceGroupGenerator(size_t number, std::string hmSymbol, std::string generatorInformation);
  virtual ~AbstractSpaceGroupGenerator() = default;

  inline size_t getNumber() const { return m_number; }
  const inline std::string &getHMSymbol() const { return m_hmSymbol; }
  const inline std::string &getGeneratorString() const { return m_generatorString; }

  SpaceGroup_const_sptr getPrototype();

protected:
  virtual Group_const_sptr generateGroup() const = 0;

private:
  inline bool hasValidPrototype() const { return static_cast<bool>(m_prototype); }
  SpaceGroup_const_sptr generatePrototype();

  size_t m_number;
  std::string m_hmSymbol;
  std::string m_generatorString;

  SpaceGroup_const_sptr m_prototype;
};

using AbstractSpaceGroupGenerator_sptr = std::shared_ptr<AbstractSpaceGroupGenerator>;

/// Concrete space group generator that uses space group generators as given in
/// ITA.
class MANTID_GEOMETRY_DLL AlgorithmicSpaceGroupGenerator : public AbstractSpaceGroupGenerator {
public:
  AlgorithmicSpaceGroupGenerator(size_t number, const std::string &hmSymbol, const std::string &generatorInformation);

protected:
  Group_const_sptr generateGroup() const override;
  std::string getCenteringSymbol() const;
};

/// Concrete generator that generates a space group from another space group
/// using a transformation.
class MANTID_GEOMETRY_DLL TransformationSpaceGroupGenerator : public AbstractSpaceGroupGenerator {
public:
  TransformationSpaceGroupGenerator(size_t number, const std::string &hmSymbol,
                                    const std::string &generatorInformation);

protected:
  Group_const_sptr generateGroup() const override;
  virtual SpaceGroup_const_sptr getBaseSpaceGroup() const;

  void setBaseAndTransformation(const std::string &generatorInformation);

  std::string m_baseGroupHMSymbol;
  std::string m_transformation;
};

/// Concrete space group generator that constructs space groups from a list of
/// symmetry operations with no further computations.
class MANTID_GEOMETRY_DLL TabulatedSpaceGroupGenerator : public AbstractSpaceGroupGenerator {
public:
  TabulatedSpaceGroupGenerator(size_t number, const std::string &hmSymbol, const std::string &generatorInformation);

protected:
  Group_const_sptr generateGroup() const override;
};

/**
  @class SpaceGroupFactory

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

  A third option is available, using a TransformationSpaceGroupGenerator,
  which generates a space group using the factory and transforms it using
  the specified transformation.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 08/10/2014
*/
class MANTID_GEOMETRY_DLL SpaceGroupFactoryImpl {
public:
  virtual ~SpaceGroupFactoryImpl() = default;

  SpaceGroup_const_sptr createSpaceGroup(const std::string &hmSymbol);

  bool isSubscribed(const std::string &hmSymbol) const;
  bool isSubscribed(size_t number) const;

  std::vector<std::string> subscribedSpaceGroupSymbols() const;
  std::vector<std::string> subscribedSpaceGroupSymbols(size_t number) const;
  std::vector<size_t> subscribedSpaceGroupNumbers() const;

  std::vector<std::string> subscribedSpaceGroupSymbols(const PointGroup_sptr &pointGroup);

  void unsubscribeSpaceGroup(const std::string &hmSymbol);

  void subscribeGeneratedSpaceGroup(size_t number, const std::string &hmSymbol, const std::string &generators);
  void subscribeTabulatedSpaceGroup(size_t number, const std::string &hmSymbol, const std::string &symmetryOperations);

  /// Templated method to subscribe other generators than the ones provided
  /// here.
  template <typename T>
  void subscribeUsingGenerator(size_t number, const std::string &hmSymbol, const std::string &generatorString) {
    if (isSubscribed(hmSymbol)) {
      throw std::invalid_argument("Space group with symbol '" + hmSymbol + "' is already registered.");
    }

    AbstractSpaceGroupGenerator_sptr generator = std::make_shared<T>(number, hmSymbol, generatorString);

    subscribe(generator);
  }

  /**
   * Specialized method to subscribe an orthorhombic space group
   *
   * For each orthorhombic space group there may be 6 different settings
   * resulting from the permutation of axes. Instead of supplying all of
   * them manually it's enough to supply generators for the standard setting
   * and the other settings (if they exist, for space groups like P222
   * all 6 are the same) are then generated automatically using the
   * transformation matrices given in table 5.1.3.1 in ITA (p. 80).
   *
   * @param number :: Space group number (ITA)
   * @param hmSymbol :: Herrman-Mauguin symbol (standard setting)
   * @param generatorString :: Generating symmetry operations (standard setting)
   */
  template <typename T>
  void subscribeOrthorhombicSpaceGroup(size_t number, const std::string &hmSymbol, const std::string &generatorString) {
    // Subscribe the base type, this must always be done.
    subscribeUsingGenerator<T>(number, hmSymbol, generatorString);

    /* For each orthorhombic space group there are in principle 6 permutations.
     * The other 5 can be constructed by TransformationSpaceGroupGenerator,
     * using the following transformations.
     */
    std::vector<std::string> transformations{"y,x,-z", "y,z,x", "z,y,-x", "z,x,y", "x,z,-y"};
    /* For some space groups, some (or all) transformations lead to the same
     * space
     * group, it's necessary to keep track of this.
     */
    std::vector<std::string> transformedSpaceGroupSymbols;

    for (const auto &transformation : transformations) {
      std::string transformedSymbol = getTransformedSymbolOrthorhombic(hmSymbol, transformation);

      bool symbolExists = std::find(transformedSpaceGroupSymbols.cbegin(), transformedSpaceGroupSymbols.cend(),
                                    transformedSymbol) != transformedSpaceGroupSymbols.cend();

      if (transformedSymbol != hmSymbol && !symbolExists) {
        subscribeUsingGenerator<TransformationSpaceGroupGenerator>(
            number, transformedSymbol, std::string(hmSymbol).append("|").append(transformation));
        transformedSpaceGroupSymbols.emplace_back(transformedSymbol);
      }
    }
  }

protected:
  std::string getTransformedSymbolOrthorhombic(const std::string &hmSymbol, const std::string &transformation) const;

  SpaceGroup_const_sptr getPrototype(const std::string &hmSymbol);
  void subscribe(const AbstractSpaceGroupGenerator_sptr &generator);
  SpaceGroup_const_sptr constructFromPrototype(const SpaceGroup_const_sptr &prototype) const;

  void fillPointGroupMap();

  std::multimap<size_t, std::string> m_numberMap;
  std::map<std::string, AbstractSpaceGroupGenerator_sptr> m_generatorMap;
  std::multimap<std::string, std::string> m_pointGroupMap;

  SpaceGroupFactoryImpl();

private:
  friend struct Mantid::Kernel::CreateUsingNew<SpaceGroupFactoryImpl>;
};

using SpaceGroupFactory = Mantid::Kernel::SingletonHolder<SpaceGroupFactoryImpl>;

} // namespace Geometry
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_GEOMETRY template class MANTID_GEOMETRY_DLL
    Mantid::Kernel::SingletonHolder<Mantid::Geometry::SpaceGroupFactoryImpl>;
}
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

#define DECLARE_GENERATED_SPACE_GROUP(number, hmSymbol, generators)                                                    \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper SPGF_CONCAT(register_spacegroup_, __COUNTER__)(                                   \
      ((Mantid::Geometry::SpaceGroupFactory::Instance().subscribeGeneratedSpaceGroup(number, hmSymbol, generators)),   \
       0));                                                                                                            \
  }

#define DECLARE_TRANSFORMED_SPACE_GROUP(number, hmSymbol, generators)                                                  \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper SPGF_CONCAT(register_spacegroup_, __COUNTER__)(                                   \
      ((Mantid::Geometry::SpaceGroupFactory::Instance().subscribeUsingGenerator<TransformationSpaceGroupGenerator>(    \
           number, hmSymbol, generators)),                                                                             \
       0));                                                                                                            \
  }

#define DECLARE_TABULATED_SPACE_GROUP(number, hmSymbol, symmetryOperations)                                            \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper SPGF_CONCAT(register_spacegroup_, __COUNTER__)(                                   \
      ((Mantid::Geometry::SpaceGroupFactory::Instance().subscribeTabulatedSpaceGroup(number, hmSymbol,                 \
                                                                                     symmetryOperations)),             \
       0));                                                                                                            \
  }

#define DECLARE_ORTHORHOMBIC_SPACE_GROUP(number, hmSymbol, generators)                                                 \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper SPGF_CONCAT(register_spacegroup_, __COUNTER__)(                                   \
      ((Mantid::Geometry::SpaceGroupFactory::Instance()                                                                \
            .subscribeOrthorhombicSpaceGroup<AlgorithmicSpaceGroupGenerator>(number, hmSymbol, generators)),           \
       0));                                                                                                            \
  }

#define DECLARE_TRANSFORMED_ORTHORHOMBIC_SPACE_GROUP(number, hmSymbol, generators)                                     \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper SPGF_CONCAT(register_spacegroup_, __COUNTER__)(                                   \
      ((Mantid::Geometry::SpaceGroupFactory::Instance()                                                                \
            .subscribeOrthorhombicSpaceGroup<TransformationSpaceGroupGenerator>(number, hmSymbol, generators)),        \
       0));                                                                                                            \
  }
