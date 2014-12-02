#ifndef MANTID_GEOMETRY_SPACEGROUPFACTORY_H_
#define MANTID_GEOMETRY_SPACEGROUPFACTORY_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidGeometry/Crystal/SpaceGroup.h"
#include "MantidKernel/RegistrationHelper.h"

#include <map>

namespace Mantid
{
namespace Geometry
{

/// Helper class for subscribing space groups.
struct MANTID_GEOMETRY_DLL SpaceGroupSubscriptionHelper
{
    enum GenerationMethod {
        Tabulated,
        Generated
    };

    SpaceGroupSubscriptionHelper(size_t number,
                                 const std::string &hmSymbol,
                                 const std::string &generationInformation,
                                 GenerationMethod generationMethod) :
        number(number),
        hmSymbol(hmSymbol),
        generationInformation(generationInformation),
        generationMethod(generationMethod)
    { }

    size_t number;
    std::string hmSymbol;
    std::string generationInformation;
    GenerationMethod generationMethod;
};

bool MANTID_GEOMETRY_DLL isValidGeneratorString(const std::string &generatorString);

class MANTID_GEOMETRY_DLL AbstractSpaceGroupGenerator
{
public:
    AbstractSpaceGroupGenerator(size_t number, const std::string &hmSymbol, const std::string &generatorInformation);
    virtual ~AbstractSpaceGroupGenerator() { }

    inline size_t getNumber() const { return m_number; }
    inline std::string getHMSymbol() const { return m_hmSymbol; }
    inline std::string getGeneratorString() const { return m_generatorString; }

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

class MANTID_GEOMETRY_DLL AlgorithmicSpaceGroupGenerator : public AbstractSpaceGroupGenerator
{
public:
    AlgorithmicSpaceGroupGenerator(size_t number, const std::string &hmSymbol, const std::string &generatorInformation);
    virtual ~AlgorithmicSpaceGroupGenerator() { }

protected:
    Group_const_sptr generateGroup() const;
    std::string getCenteringSymbol() const;

};

class MANTID_GEOMETRY_DLL TabulatedSpaceGroupGenerator : public AbstractSpaceGroupGenerator
{
public:
    TabulatedSpaceGroupGenerator(size_t number, const std::string &hmSymbol, const std::string &generatorInformation);
    virtual ~TabulatedSpaceGroupGenerator() { }

protected:
    Group_const_sptr generateGroup() const;
};

/** SpaceGroupFactory

  Factory for SpaceGroups. When a space group is subscribed, it
  creates a prototype object and stores that. All space group
  creations through the factory are just copy-constructions.

  Space groups can be subscribed using one of two available methods,
  either by supplying ALL symmetry operations (this is called a "tabulated"
  space group) or a set of generators (from International Tables for
  Crystallography A).

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
class MANTID_GEOMETRY_DLL SpaceGroupFactoryImpl
{
public:
    virtual ~SpaceGroupFactoryImpl() { }

    SpaceGroup_const_sptr createSpaceGroup(const std::string &hmSymbol);

    bool isSubscribed(const std::string &hmSymbol) const;
    bool isSubscribed(size_t number) const;

    std::vector<std::string> subscribedSpaceGroupSymbols() const;
    std::vector<std::string> subscribedSpaceGroupSymbols(size_t number) const;
    std::vector<size_t> subscribedSpaceGroupNumbers() const;

    void unsubscribeSpaceGroup(const std::string &hmSymbol);

    void subscribeGeneratedSpaceGroup(size_t number, const std::string &hmSymbol, const std::string &generators);
    void subscribeTabulatedSpaceGroup(size_t number, const std::string &hmSymbol, const std::string &symmetryOperations);

protected:
    void throwIfSubscribed(const std::string &hmSymbol);

    SpaceGroup_const_sptr generateValidPrototype(const std::string &hmSymbol) const;
    Group_const_sptr generateValidGeneratedGroup(const std::string &hmSymbol, const std::string &generators) const;
    Group_const_sptr generateValidTabulatedGroup(const std::string &generators) const;

    SpaceGroup_const_sptr getPrototype(const std::string &hmSymbol);
    void registerValidPrototype(const SpaceGroup_const_sptr &prototype);

    std::string getCenteringString(const std::string &hmSymbol) const;
    Group_const_sptr getGeneratedGroup(const std::string &generators, const std::string &centeringSymbol) const;
    Group_const_sptr getTabulatedGroup(const std::string &symmetryOperations) const;

    SpaceGroup_const_sptr getPrototype(Group_const_sptr generatingGroup, size_t number, const std::string &hmSymbol) const;

    void subscribeGeneratorInformation(size_t number, const std::string &hmSymbol, const std::string &generators, SpaceGroupSubscriptionHelper::GenerationMethod method);
    bool isValidGeneratorString(const std::string &generatorString) const;
    void subscribe(const SpaceGroup_const_sptr &prototype);

    SpaceGroup_const_sptr constructFromPrototype(const SpaceGroup_const_sptr prototype) const;

    std::multimap<size_t, std::string> m_numberMap;
    std::map<std::string, SpaceGroupSubscriptionHelper> m_generatorMap;
    std::map<std::string, SpaceGroup_const_sptr> m_prototypes;

    SpaceGroupFactoryImpl();

private:
    friend struct Mantid::Kernel::CreateUsingNew<SpaceGroupFactoryImpl>;
};

// This is taken from FuncMinimizerFactory
#ifdef _WIN32
  template class MANTID_GEOMETRY_DLL Mantid::Kernel::SingletonHolder<SpaceGroupFactoryImpl>;
#endif

typedef Mantid::Kernel::SingletonHolder<SpaceGroupFactoryImpl> SpaceGroupFactory;


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

#define DECLARE_GENERATED_SPACE_GROUP(number,hmSymbol,generators) \
      namespace { \
  Mantid::Kernel::RegistrationHelper SPGF_CONCAT(register_spacegroup_, __COUNTER__)( \
((Mantid::Geometry::SpaceGroupFactory::Instance().subscribeGeneratedSpaceGroup(number, hmSymbol, generators)) \
  , 0)); \
  }

#define DECLARE_TABULATED_SPACE_GROUP(number,hmSymbol,symmetryOperations) \
    namespace { \
Mantid::Kernel::RegistrationHelper SPGF_CONCAT(register_spacegroup_, __COUNTER__)( \
((Mantid::Geometry::SpaceGroupFactory::Instance().subscribeTabulatedSpaceGroup(number, hmSymbol, symmetryOperations)) \
, 0)); \
}

#endif  /* MANTID_GEOMETRY_SPACEGROUPFACTORY_H_ */
