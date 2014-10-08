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

    SpaceGroup_const_sptr createSpaceGroup(const std::string &hmSymbol) const;
    SpaceGroup_const_sptr createSpaceGroup(size_t number) const;

    bool isSubscribed(const std::string &hmSymbol) const;
    bool isSubscribed(size_t number) const;

    std::vector<std::string> subscribedSpaceGroupSymbols() const;
    std::vector<size_t> subscribedSpaceGroupNumbers() const;

    void unsubscribeSpaceGroup(const std::string &hmSymbol);
    void unsubscribeSpaceGroup(size_t number);

    void subscribeGeneratedSpaceGroup(size_t number, const std::string &hmSymbol, const std::string &generators);
    void subscribeTabulatedSpaceGroup(size_t number, const std::string &hmSymbol, const std::string &symmetryOperations);

protected:
    void throwIfSubscribed(size_t number, const std::string &hmSymbol);

    std::string getCenteringString(const std::string &hmSymbol) const;
    Group_const_sptr getGeneratedGroup(const std::string &generators, const std::string &centeringSymbol) const;
    Group_const_sptr getTabulatedGroup(const std::string &symmetryOperations) const;

    SpaceGroup_const_sptr getPrototype(Group_const_sptr generatingGroup, size_t number, const std::string &hmSymbol) const;

    void subscribe(const SpaceGroup_const_sptr &prototype);

    SpaceGroup_const_sptr constructFromPrototype(const SpaceGroup_const_sptr prototype) const;

    std::map<size_t, SpaceGroup_const_sptr> m_prototypesByNumber;
    std::map<std::string, SpaceGroup_const_sptr> m_prototypesBySymbol;

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

#define DECLARE_GENERATED_SPACE_GROUP(number,hmSymbol,generators) \
      namespace { \
  Mantid::Kernel::RegistrationHelper register_symop_##number( \
((Mantid::Geometry::SpaceGroupFactory::Instance().subscribeGeneratedSpaceGroup(number, hmSymbol, generators)) \
  , 0)); \
  }

#define DECLARE_TABULATED_SPACE_GROUP(number,hmSymbol,symmetryOperations) \
      namespace { \
  Mantid::Kernel::RegistrationHelper register_symop_##number( \
((Mantid::Geometry::SpaceGroupFactory::Instance().subscribeTabulatedSpaceGroup(number, hmSymbol, symmetryOperations)) \
  , 0)); \
  }

#endif  /* MANTID_GEOMETRY_SPACEGROUPFACTORY_H_ */
